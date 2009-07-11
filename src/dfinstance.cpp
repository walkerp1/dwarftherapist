#include <QtDebug>
#include <QApplication>
#include <QVector>
#include <QByteArrayMatcher>
#include <windows.h>
#include <psapi.h>

#include "win_structs.h"
#include "dfinstance.h"
#include "dwarf.h"
#include "utils.h"


DFInstance::DFInstance(DWORD pid, HWND hwnd, QObject* parent)
    :QObject(parent)
    ,m_pid(pid)
    ,m_hwnd(hwnd)
    ,m_proc(0)
    ,m_memory_correction(0)
    ,m_stop_scan(false)
{
    m_proc = OpenProcess(PROCESS_QUERY_INFORMATION
                         | PROCESS_VM_READ
                         | PROCESS_VM_OPERATION
                         | PROCESS_VM_WRITE, false, m_pid);

    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(m_proc, &pmc, sizeof(pmc));
    m_memory_size = pmc.WorkingSetSize;
    qDebug() << "process working set size: " << m_memory_size;

    PVOID peb_addr = GetPebAddress(m_proc);
    qDebug() << "PEB is at: " << hex << peb_addr;

	PEB peb;
	DWORD bytes = 0;
	if (ReadProcessMemory(m_proc, (PCHAR)peb_addr, &peb, sizeof(PEB), &bytes)) {
		qDebug() << "read " << bytes << "bytes BASE ADDR is at: " << hex << peb.ImageBaseAddress;
		m_base_addr = (int)peb.ImageBaseAddress;
	} else {
		qCritical() << "unable to read remote PEB!" << GetLastError();
	}

	calculate_checksum();

    m_memory_correction = (int)m_base_addr - 0x0400000;
    qDebug() << "memory correction " << m_memory_correction;
}

DFInstance::~DFInstance() {
    if (m_proc) {
        CloseHandle(m_proc);
    }
}

int DFInstance::calculate_checksum() {
	uint bytes_read = 0;
	char expect_M = read_char(m_base_addr, bytes_read);
	char expect_Z = read_char(m_base_addr + 0x1, bytes_read);

	if (expect_M != 'M' || expect_Z != 'Z') {
		qWarning() << "invalid executable";
	}
	uint pe_header = m_base_addr + read_int32(m_base_addr + 30 * 2, bytes_read);
	char expect_P = read_char(pe_header, bytes_read);
	char expect_E = read_char(pe_header + 0x1, bytes_read);
	if (expect_P != 'P' || expect_E != 'E') {
		qWarning() << "PE header invalid";
	}

	int timestamp = read_int32(pe_header + 4 + 2 * 2, bytes_read);
	return timestamp;
}


QString DFInstance::get_base_address() {
    return QString("%1").arg((int)m_base_addr);
}

QString DFInstance::read_string(int address) {
    uint bytes_read = 0;
    int len = read_int32(address + STRING_LENGTH_OFFSET, bytes_read);
    int cap = read_int32(address + STRING_CAP_OFFSET, bytes_read);
    int buffer_addr = address + STRING_BUFFER_OFFSET;
    if (cap >= 16)
        buffer_addr = read_int32(buffer_addr, bytes_read);

	Q_ASSERT(len <= cap);
	Q_ASSERT(len >= 0);
	Q_ASSERT(len < (1 << 16));
    
    char *buffer = new char[len];
    bytes_read = read_raw(buffer_addr, len, buffer);
	QString ret_val = QString::fromAscii(buffer, bytes_read);
    delete[] buffer;
    return ret_val;
}

short DFInstance::read_short(int start_address, uint &bytes_read) {
    short val = 0;
    bytes_read = 0;
    ReadProcessMemory(m_proc, (LPCVOID)start_address, &val, sizeof(short), (DWORD*)&bytes_read);
    //qDebug() << "Read from " << start_address << "OK?" << ok <<  " bytes read: " << bytes_read << " VAL " << hex << val;
    return val;
}

int DFInstance::read_int32(int start_address, uint &bytes_read) {
    int val = 0;
    bytes_read = 0;
    ReadProcessMemory(m_proc, (LPCVOID)start_address, &val, sizeof(int), (DWORD*)&bytes_read);
    //qDebug() << "Read from " << start_address << "OK?" << ok <<  " bytes read: " << bytes_read << " VAL " << hex << val;
    return val;
}

//returns # of bytes read
char DFInstance::read_char(int start_address, uint &bytes_read) {
    char val = 0;
	bytes_read = 0;
    ReadProcessMemory(m_proc, (LPCVOID)start_address, &val, sizeof(char), (DWORD*)&bytes_read);
    //qDebug() << "Read from " << start_address << "OK?" << ok <<  " bytes read: " << bytes_read << " VAL " << val;
    return val;
}

int DFInstance::read_raw(int start_address, int bytes, char *buffer) {
	memset(buffer, 0, bytes);
	DWORD bytes_read = 0;
	ReadProcessMemory(m_proc, (LPCVOID)start_address, (void*)buffer, sizeof(char) * bytes, &bytes_read);
	return bytes_read;
}

QVector<int> DFInstance::scan_mem_find_all(QByteArray &needle, int start_address, int end_address) {
	QVector<int> addresses;
	bool ok;
	do {
		int addr = scan_mem(needle, start_address, end_address, ok);
		if (ok && addr != 0) {
			addresses.push_back(addr);
			start_address = addr+1;
		} else {
			break;
		}
	} while (start_address < end_address);
	return addresses;
}

int DFInstance::scan_mem(QByteArray &needle, int start_address, int end_address, bool &ok) {
    m_stop_scan = false;
    ok = true;
    if (end_address <= start_address) {
        qWarning() << "start address must be lower than end_address";
        ok = false;
        return 0;
    }
    //qDebug() << "starting scan from" << hex << start_address << "to" << end_address << "for" << needle;

    int step = 0x1000;
    int bytes_read = 0;
    char buffer[0x1000];
    int ptr = start_address;

    int total_steps = (end_address - start_address) / step;
    if ((end_address - start_address) % step) {
        total_steps++;
    }
    emit scan_total_steps(total_steps);
	emit scan_progress(0);

	QByteArrayMatcher matcher(needle);
    for (int i = 0; i < total_steps; ++i) {
        if (m_stop_scan) {
            break;
        }
        ptr = start_address + (i * step);
        bytes_read = read_raw(ptr, step, (char*)&buffer[0]);
		
		int idx = matcher.indexIn(QByteArray((char*)&buffer[0], bytes_read));
        if (idx != -1) {
            //qDebug() << "FOUND" << needle << "at" << hex << ptr + idx;
            ok = true;
            //delete[] buffer;
            emit scan_progress(total_steps-1); // don't finish it all the way
            return ptr + idx;
        }
        emit scan_progress(i);
        qApp->processEvents();
    }
    //delete[] buffer;
    ok = false;
    return 0;
}

QVector<int> DFInstance::enumerate_vector(int address) {
	QVector<int> addresses;
	uint bytes_read = 0;
    int start = read_int32(address + 4, bytes_read);
    int end = read_int32(address + 8, bytes_read);

	Q_ASSERT(end >= start);
	Q_ASSERT((end - start) % 4 == 0);
	Q_ASSERT(start % 4 == 0);
	Q_ASSERT(end % 4 == 0);

	for( int ptr = start; ptr < end; ptr += 4 ) {
		int addr = read_int32(ptr, bytes_read);
		if (bytes_read == sizeof(int)) {
			addresses.append(addr);
		}
	}
	return addresses;
}

QVector<QVector<int> > DFInstance::cross_product(QVector<QVector<int> > addresses, int index) {
    QVector<QVector<int> > out;
    return out;
    /*
    if (index >= addresses.size()) {
        QVector<int> foo;
        foo.reserve(index);
        out.append(foo);
        return out;
    } else {
        foreach(int i, addresses[index]) {
            foreach(QVector<int> p, cross_product(addresses, index + 1)) {
                p[index] = i;
                return p;
            }
        }
    }
    return out;
    */
}

/*
public IEnumerable<int[]> CrossProduct( List<int>[] addresses, int idx )
{
    if( idx >= addresses.Length ) {
        yield return new int[idx];
    } else {
        foreach( int i in addresses[idx] ) {
            foreach( int[] p in CrossProduct( addresses, idx + 1 ) ) {
                p[idx] = i;
                yield return p;
            }
        }
    }
}*/

int DFInstance::find_language_vector() {
    // TODO: move to config
    int language_word_number = 1373;
    int lang_vector_low_cutoff = 0x01500000;
    int lang_vector_high_cutoff = 0x015D0000;
    QByteArray needle("LANCER");


    int language_vector_address = -1; // return val
    emit scan_message(tr("Scanning for known word"));
    foreach(int word_addr, scan_mem_find_all(needle, 0, m_memory_size)) {
        emit scan_message(tr("Scanning for word pointer"));
        qDebug() << "FOUND WORD" << hex << word_addr;
        needle = encode_int(word_addr - 4);
        foreach(int word_addr_ptr, scan_mem_find_all(needle, 0, m_memory_size)) {
            emit scan_message(tr("Scanning for language vector"));
            qDebug() << "FOUND WORD PTR" << hex << word_addr_ptr;
            int word_list = word_addr_ptr - (language_word_number * 4);
            needle = encode_int(word_list);
            foreach(int word_list_ptr, scan_mem_find_all(needle, lang_vector_low_cutoff + m_memory_correction,
                                                         lang_vector_high_cutoff + m_memory_correction)) {
                qDebug() << "LANGUAGE VECTOR IS AT" << hex << word_list_ptr - 0x4;
                language_vector_address = word_list_ptr - 0x4;
            }
        }
    }
    qDebug() << "all done with language scan";
    return language_vector_address;
}

int DFInstance::find_translation_vector() {
    // TODO: move to config
    int translation_vector_low_cutoff = 0x01500000 + m_memory_correction;
    int translation_vector_high_cutoff = 0x015D0000 + m_memory_correction;
    int translation_word_number = 1373;
    int translation_number = 0;
    QByteArray translation_word("kivish");
    QByteArray translation_name("DWARF");

    int translation_vector_address = -1; //return val;
    emit scan_message(tr("Scanning for translation vector"));
    QByteArray needle(translation_word);
    foreach(int word, scan_mem_find_all(translation_word, 0, m_memory_size)) {
        needle = encode_int(word - 4);
        foreach(int word_ptr, scan_mem_find_all(needle, 0, m_memory_size)) {
            needle = encode_int(word_ptr - (translation_word_number * 4));
            foreach(int word_list_ptr, scan_mem_find_all(needle, 0, m_memory_size)) {
                needle = translation_name;
                foreach(int dwarf_translation_name, scan_mem_find_all(needle, word_list_ptr - 0x1000, word_list_ptr)) {
                    dwarf_translation_name -= 4;
                    qDebug() << "FOUND TRANSLATION WORD TABLE" << hex << word_list_ptr - dwarf_translation_name;
                    needle = encode_int(dwarf_translation_name);
                    foreach(int dwarf_translation_ptr, scan_mem_find_all(needle, 0, m_memory_size)) {
                        int translations_list = dwarf_translation_ptr - (translation_number * 4);
                        needle = encode_int(translations_list);
                        foreach(int translations_list_ptr, scan_mem_find_all(needle,
                                                                             translation_vector_low_cutoff,
                                                                             translation_vector_high_cutoff)) {
                            translation_vector_address = translations_list_ptr - 4;
                            qDebug() << "FOUND TRANSLATIONS VECTOR" << hex << translation_vector_address;
                        }
                    }
                }
            }
        }
    }
    qDebug() << "all done with translation scan";
    return translation_vector_address;
}

int DFInstance::find_creature_vector() {
    // TODO: move to config
    int creature_vector_low_cutoff = 0x01300000 + m_memory_correction;
    int creature_vector_high_cutoff = 0x01700000 + m_memory_correction;
    int dwarf_nickname_offset = 0x001C;
    QByteArray custom_nickname("FirstCreature");
    QByteArray custom_profession("FirstProfession");

    QByteArray skillpattern_miner = encode_skillpattern(0, 3340, 4);
    QByteArray skillpattern_metalsmith = encode_skillpattern(29, 0, 2);
    QByteArray skillpattern_swordsman = encode_skillpattern(40, 462, 3);
    QByteArray skillpattern_pump_operator = encode_skillpattern(65, 462, 1);

	uchar foo[6] = {0x00, 0x00, 0x0C, 0x0D, 0x04, 0x00};
	skillpattern_miner = QByteArray((char*)&foo, 6);


    int creature_vector_address = -1;
    /*

    TURN THIS BACK ON!

    emit scan_message(tr("Scanning for known nickname"));
    QByteArray needle(custom_nickname);
    int dwarf;
    foreach(int nickname, scan_mem_find_all(needle, 0, m_memory_size)) {
        qDebug() << "FOUND NICKNAME" << hex << nickname;
        emit scan_message(tr("Scanning for dwarf objects"));
        needle = encode_int(nickname - dwarf_nickname_offset - 4); // should be the address of this dwarf
        foreach(int dwarf, scan_mem_find_all(needle, 0, m_memory_size)) {
            qDebug() << "FOUND DWARF" << hex << dwarf;
            emit scan_message(tr("Scanning for dwarf vector pointer"));
            needle = encode_int(dwarf); // since this is the first dwarf, it should also be the dwarf vector
            foreach(int vector_ptr, scan_mem_find_all(needle, creature_vector_low_cutoff, creature_vector_high_cutoff)) {
                creature_vector_address = vector_ptr - 0x4;
                qDebug() << "FOUND CREATURE VECTOR" << hex << creature_vector_address;
            }
        }
    }

    needle = custom_profession;
    foreach(int prof, scan_mem_find_all(needle, dwarf, dwarf + 0x1000)) {
        qDebug() << "Custom Profession Offset" << prof - dwarf - 4;
    }

    */

    QVector<QByteArray> patterns;
    patterns.append(skillpattern_miner);
    patterns.append(skillpattern_metalsmith);
    patterns.append(skillpattern_swordsman);
    patterns.append(skillpattern_pump_operator);

    QVector< QVector<int> > addresses;
    addresses.reserve(patterns.size());
    for (int i = 0; i < 4; ++i) {
        QVector<int> lst;
        emit scan_message("scanning for skill pattern" + QString::number(i));
		foreach(int skill, scan_mem_find_all(patterns[i], 0, m_memory_size + m_base_addr - m_memory_correction)) {
            qDebug() << "FOUND SKILL PATTERN" << i << by_char(patterns[i]) << "AT" << hex << skill;
            lst.append(skill);
        }
		addresses.append(lst);
		break;
    }
    /*
                List<int>[] addresses = new List<int>[config.SkillPatterns.Length];
                for( int i = 0; i < config.SkillPatterns.Length; i++ ) {
                    List<int> list = new List<int>();
                    foreach( int skill in FindPattern( config.SkillPatterns[i] ) )
                        list.Add( skill );
                    addresses[i] = list;
                }
                foreach( int addr in addresses[0] ) {
                    foreach( int start in Find( addr ) ) {
                        byte[] buffer = memoryAccess.ReadMemory( start, config.SkillPatterns.Length*4 );
                        foreach( int[] p in CrossProduct( addresses, 0 ) ) {
                            Matcher matcher = new Matcher( BytesHelper.ToBytes( p ) );
                            if( matcher.Matches( ref buffer, 0, buffer.Length ) ) {
                                foreach( int listPointers in Find( start, dwarf, dwarf + 0x2000 ) ) {
                                    ReportAddress( "Offset", "Creature.SkillVector", listPointers - dwarf - 4 );
                                }
                            }
                        }
                    }
                }
                foreach( int labors in FindPattern( config.LaborsPattern, dwarf, dwarf + 0x2000 ) ) {
                    ReportAddress( "Offset", "Creature.Labors", labors - dwarf );
                }
    */
	qDebug() << "all done with creature scan";
    return creature_vector_address;
}

DFInstance* DFInstance::find_running_copy(QObject *parent) {
    HWND hwnd = FindWindow(NULL, L"Dwarf Fortress");
    if (!hwnd) {
        qWarning() << "can't find running copy";
        return 0;
    }
    qDebug() << "found copy with HWND: " << hwnd;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    qDebug() << "PID is: " << pid;

    return new DFInstance(pid, hwnd, parent);
}

QVector<Dwarf*> DFInstance::load_dwarves() {
	int a = 0x223b3cc; // home 1
	int b = 0x223b354; // home 2
	int c = 0x013ab3cc; // from work
	int d = 0x0151b354; // from file
	QVector<Dwarf*> dwarves;
	QVector<int> creatures = enumerate_vector(d + m_memory_correction);
	if (creatures.size() > 0) {
		for (int offset=0; offset < creatures.size(); ++offset) {
			Dwarf *d = Dwarf::get_dwarf(this, creatures[offset]);
			if (d) {
				dwarves.append(d);
				qDebug() << "CREATURE" << offset << d->to_string();
			} else {
				qWarning() << "BOGUS CREATURE" << offset;
			}
		}
	}
	qDebug() << "finished reading creature vector";
	return dwarves;
}
