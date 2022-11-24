#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <unordered_map>
using namespace std;

// #define DISK_SIZE 524288000
// #define BLOCK_SIZE 4096
// #define INODE_ALLOCATION 60

#define DISK_SIZE 524288000
#define BLOCK_SIZE 4096
#define INODE_ALLOCATION 50
#define FD_LIMIT 32
#define NO_OF_BLOCKS 128000
#define NO_OF_INODES 64000

struct file_inode_entry
{
	char filename[20];
	int inode_number = -1;
};

struct inode
{
	int filesize = -1;
	int pointers[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

};

struct superblock
{
	int no_of_blocks = NO_OF_BLOCKS;
	int no_of_inode_blocks = NO_OF_INODES;
	int superblock_block_no = ceil(float(sizeof(superblock)) / float(BLOCK_SIZE));
	int file_table_size = (no_of_inode_blocks* sizeof(file_inode_entry));
	int file_table_block_no = ceil(float(file_table_size) / float(BLOCK_SIZE));

	int super_block_start = 0;
	int file_table_start = superblock_block_no;
	int inode_start = file_table_start + file_table_block_no;
	int disk_start = inode_start + no_of_inode_blocks;
	bool block_state[NO_OF_BLOCKS];
};

unordered_map<string, int> file_table;
int no_of_blocks = ceil(float(DISK_SIZE) / float(BLOCK_SIZE));
int no_of_inodes = ceil((float(no_of_blocks) *float(INODE_ALLOCATION)) / 100);
struct inode list_of_inodes[NO_OF_INODES];
struct superblock current_sb;
FILE * current_disk;
string disk_name;
vector<bool> fd_list(FD_LIMIT, false);
unordered_map<int, string> fd_to_filename_map;
unordered_map<string, vector < int>> filename_to_fd_map;
unordered_map<int, int> file_mode;
int open_file_count = 0;

bool disk_exists(string diskname)
{
	ifstream f(diskname.c_str());
	return f.good();
}

void display_superblock()
{
	cout << current_sb.no_of_blocks << endl;
	cout << current_sb.no_of_inode_blocks << endl;
	cout << current_sb.super_block_start << endl;
	cout << current_sb.file_table_start << endl;
	cout << current_sb.inode_start << endl;
	cout << current_sb.disk_start << endl;
	for (int i = 0; i < current_sb.no_of_blocks; i++)
		cout << current_sb.block_state[i];
	cout << endl;
}

void display_internal_structres()
{
	cout << "SUPERBLOCK" << endl;
	display_superblock();
	cout << "INODE ARRAY" << endl;
	for (int i = 0; i < NO_OF_INODES; i++)
	{
		cout << list_of_inodes[i].filesize << "- ";
		for (int j = 0; j < 12; j++)
			cout << list_of_inodes[i].pointers[j] << " ";
		cout << "-" << endl;
	}

	cout << "FILETABLE" << endl;
	for (auto el: file_table)
	{
		cout << el.first << " " << el.second << endl;
	}

	cout << "FILE DESCRIPTOR" << endl;
	for (auto el: filename_to_fd_map)
	{
		cout << el.first << ": ";
		for (auto i: el.second)
			cout << i << " ";
		cout << endl;
	}
}

int get_file_action()
{
	int option;
	cout << "=====================================" << endl;
	cout << "FILE MENU" << endl;
	cout << "=====================================" << endl;
	cout << "1 : create file" << endl;
	cout << "2 : open file" << endl;
	cout << "3 : read file" << endl;
	cout << "4 : write file" << endl;
	cout << "5 : append file" << endl;
	cout << "6 : close file" << endl;
	cout << "7 : delete file" << endl;
	cout << "8 : list of files" << endl;
	cout << "9 : list of opened files" << endl;
	cout << "10: unmount" << endl;
	cout << "11: dump_tables" << endl;
	cout << "Enter option : ";
	cin >> option;
	while (option < 1 || option > 11)
	{
		cout << "Invalid option!" << endl;
		cout << "=====================================" << endl;
		cout << "FILE MENU" << endl;
		cout << "=====================================" << endl;
		cout << "1 : create file" << endl;
		cout << "2 : open file" << endl;
		cout << "3 : read file" << endl;
		cout << "4 : write file" << endl;
		cout << "5 : append file" << endl;
		cout << "6 : close file" << endl;
		cout << "7 : delete file" << endl;
		cout << "8 : list of files" << endl;
		cout << "9 : list of opened files" << endl;
		cout << "10: unmount" << endl;
		cout << "11: dump_tables" << endl;
		cout << "Enter option : ";
		cin >> option;
	}

	return option;
}

int get_free_fd()
{
	for (int i = 0; i < FD_LIMIT; i++)
	{
		if (!fd_list[i])
			return i;
	}

	return -1;
}

int get_free_inode_block()
{
	for (int i = current_sb.inode_start; i < current_sb.no_of_blocks; i++)
	{
		if (current_sb.block_state[i] == true)
			return i;
	}

	return -1;
}

int get_free_data_block()
{
	for (int i = current_sb.disk_start; i < current_sb.no_of_blocks; i++)
	{
		if (current_sb.block_state[i] == true)
			return i;
	}

	return -1;
}

void create_file(string filename)
{
	int inode_block = get_free_inode_block();
	if (inode_block == -1)
	{
		cout << "Could not find a free inode block!" << endl;
		return;
	}

	current_sb.block_state[inode_block] = false;
	file_table[filename] = inode_block;
	struct inode fi;
	for (int i = 0; i < 12; i++)
		fi.pointers[i] = -1;
	fi.filesize = 0;
	list_of_inodes[inode_block] = fi;
}

void open_file(string filename)
{
	if (file_table.find(filename) == file_table.end())
	{
		cout << "File not found!" << endl;
		return;
	}

	int mode;
	cout << "=====================================" << endl;
	cout << "OPEN MENU" << endl;
	cout << "=====================================" << endl;
	cout << "0 : read mode" << endl;
	cout << "1 : write mode" << endl;
	cout << "2 : append mode" << endl;
	cout << "Enter mode :";
	cin >> mode;

	while (mode < 0 || mode > 2)
	{
		cout << "Invalid mode!" << endl;
		cout << "=====================================" << endl;
		cout << "OPEN MENU" << endl;
		cout << "=====================================" << endl;
		cout << "0 : read mode" << endl;
		cout << "1 : write mode" << endl;
		cout << "2 : append mode" << endl;
		cout << "Enter mode :";
		cin >> mode;
	}

	int fd = get_free_fd();
	if (fd < 0)
	{
		cout << "Unable to open file!" << endl;
		return;
	}

	file_mode[fd] = mode;
	fd_list[fd] = true;
	fd_to_filename_map[fd] = filename;
	if (filename_to_fd_map.find(filename) == filename_to_fd_map.end())
	{
		filename_to_fd_map[filename] = vector < int> { fd };

	}
	else
	{
		filename_to_fd_map[filename].push_back(fd);
	}

	open_file_count++;
	cout << "FILE DESCRIPTOR : " << fd << endl;
}

string read_block_from_file(int block_no)
{
	fseek(current_disk, block_no *BLOCK_SIZE, SEEK_SET);
	char buffer[BLOCK_SIZE + 1] = { 0 };

	int c = fread(buffer, sizeof(char), sizeof(buffer), current_disk);
	buffer[c - 1] = '\0';
	return string(buffer);
}

void write_block_to_file(string block, int block_no)
{
	char buffer[BLOCK_SIZE + 1];
	strcpy(buffer, block.c_str());

	fseek(current_disk, block_no *BLOCK_SIZE, SEEK_SET);
	fwrite(buffer, sizeof(char), sizeof(buffer), current_disk);
}

void write_to_blocks(string input, struct inode *file_inode)
{
	int pointer_index = -1;
	for (int i = 0; i < 12; i++)
	{
		if (file_inode->pointers[i] == -1)
		{
			pointer_index = i;
			break;
		}
	}

	if (pointer_index == -1)
	{
		cout << "Exceeds max file size!" << endl;
		return;
	}

	string block;
	for (int i = 0; i < input.size(); i++)
	{
		if (i != 0 && i % BLOCK_SIZE == 0)
		{
			int block_no = get_free_data_block();
			current_sb.block_state[block_no] = false;
			write_block_to_file(block, block_no);
			file_inode->pointers[pointer_index] = block_no;
			file_inode->filesize += block.size();
			pointer_index++;
			block.erase();
		}

		block += input[i];
	}

	if (!block.empty())
	{
		int block_no = get_free_data_block();
		current_sb.block_state[block_no] = false;
		write_block_to_file(block, block_no);
		file_inode->pointers[pointer_index] = block_no;
		file_inode->filesize += block.size();
	}
}

void write_file(int fd)
{
	if (fd_to_filename_map.find(fd) == fd_to_filename_map.end())
	{
		cout << "File not opened!" << endl;
		return;
	}

	if (file_mode.find(fd) == file_mode.end() || file_mode[fd] != 1)
	{
		cout << "File not opened in write mode!" << endl;
		return;
	}

	string filename = fd_to_filename_map[fd];
	int file_inode_no = file_table[filename];
	struct inode file_inode = list_of_inodes[file_inode_no];
	for (int i = 0; i < 12; i++)
		file_inode.pointers[i] = -1;

	string input;
	string line;
	cout << endl;
	cout << "=====================================" << endl;
	cout << "ENTER TEXT[<end> to exit write mode]" << endl;
	cout << "=====================================" << endl;
	getline(cin, line);
	while (getline(cin, line))
	{
		if (line == "end")
		{
			break;
		}
		else
		{
			input += line + "\n";
		}
	}

	write_to_blocks(input, &file_inode);
	list_of_inodes[file_inode_no] = file_inode;
	cout << "=====================================" << endl;
	cout << "WRITE ENDED" << endl;
	cout << "=====================================" << endl;
	cout << endl;
}

void append_file(int fd)
{
	if (fd_to_filename_map.find(fd) == fd_to_filename_map.end())
	{
		cout << "File not opened!" << endl;
		return;
	}

	if (file_mode.find(fd) == file_mode.end() || file_mode[fd] != 2)
	{
		cout << "File not opened in append mode!" << endl;
		return;
	}

	string filename = fd_to_filename_map[fd];
	int file_inode_no = file_table[filename];
	struct inode file_inode = list_of_inodes[file_inode_no];

	string input;
	string line;
	cout << endl;
	cout << "=====================================" << endl;
	cout << "ENTER TEXT[<end> to exit write mode]" << endl;
	cout << "=====================================" << endl;
	getline(cin, line);
	while (getline(cin, line))
	{
		if (line == "end")
		{
			break;
		}
		else
		{
			input += line + "\n";
		}
	}

	write_to_blocks(input, &file_inode);
	list_of_inodes[file_inode_no] = file_inode;
	cout << "=====================================" << endl;
	cout << "WRITE ENDED" << endl;
	cout << "=====================================" << endl;
	cout << endl;
}

void read_file(int fd)
{
	if (fd_to_filename_map.find(fd) == fd_to_filename_map.end())
	{
		cout << "File not opened!" << endl;
		return;
	}

	if (file_mode.find(fd) == file_mode.end() || file_mode[fd] != 0)
	{
		cout << "File not opened in read mode!" << endl;
		return;
	}

	string filename = fd_to_filename_map[fd];
	int file_inode_no = file_table[filename];
	struct inode file_inode = list_of_inodes[file_inode_no];
	// for(int i=0;i < 12;i++)
	// 	cout << file_inode.pointers[i] << " ";
	cout << endl;
	cout << "=====================================" << endl;
	cout << "FILE CONTENT" << endl;
	cout << "=====================================" << endl;
	string content;
	for (int i = 0; i < 12; i++)
	{
		if (file_inode.pointers[i] != -1)
		{
			content += read_block_from_file(file_inode.pointers[i]);
		}
	}

	cout << content;
	cout << "=====================================" << endl;
	cout << endl;
}

void delete_file(string filename)
{
	if (file_table.find(filename) == file_table.end())
	{
		cout << "File not found!" << endl;
		return;
	}

	for (auto fd: filename_to_fd_map[filename])
	{
		fd_to_filename_map.erase(fd);
		file_mode.erase(fd);
		fd_list[fd] = false;
	}

	filename_to_fd_map.erase(filename);
	int inode_num = file_table[filename];
	current_sb.block_state[inode_num] = true;
	for (int i = 0; i < 12; i++)
	{
		list_of_inodes[inode_num].pointers[i] = -1;
	}

	file_table.erase(filename);
}

void close_file(int fd)
{
	if (fd_to_filename_map.find(fd) == fd_to_filename_map.end())
	{
		cout << "File not opened!" << endl;
		return;
	}

	string filename = fd_to_filename_map[fd];
	fd_to_filename_map.erase(fd);

	for (auto it = filename_to_fd_map[filename].begin(); it != filename_to_fd_map[filename].end(); it++)
	{
		if (*it == fd)
		{
			filename_to_fd_map[filename].erase(it);
			break;
		}
	}

	fd_list[fd] = false;
	file_mode.erase(fd);
	open_file_count--;
	cout << "=====================================" << endl;
	cout << "File closed" << endl;
	cout << "=====================================" << endl;
}

void list_open_files()
{
	cout << "=====================================" << endl;
	cout << "LIST OF OPEN FILES IN THE DISK" << endl;
	cout << "=====================================" << endl;
	for (auto el: filename_to_fd_map)
	{
		cout << el.first << " ";
		for (auto fd: el.second)
		{
			cout << "[" << fd << ":";
			int mode = file_mode[fd];
			string fmode;
			if (mode == 0)
				fmode = "READ";
			else if (mode == 1)
				fmode = "WRITE";
			else
				fmode = "APPEND";
			cout << fmode << "] ";
		}

		cout << endl;
	}

	cout << endl;
}

void list_files()
{
	cout << "=====================================" << endl;
	cout << "LIST OF FILES IN THE DISK" << endl;
	cout << "=====================================" << endl;
	for (auto file: file_table)
	{
		cout << file.first << endl;
	}

	cout << endl;
}

void unmount_disk()
{
	// SAVING SUPERBLOCK OF DISK
	char buffer[sizeof(current_sb)];
	memcpy(buffer, &current_sb, sizeof(current_sb));
	fseek(current_disk, 0, SEEK_SET);
	fwrite(buffer, sizeof(char), sizeof(current_sb), current_disk);
	cout << buffer << endl;

	// SAVING FILETABLE
	struct file_inode_entry file_table_array[current_sb.no_of_inode_blocks];
	int i = 0;
	for (auto el: file_table)
	{
		string filename = el.first;
		char *filename_array = new char[filename.size() + 1];
		strcpy(filename_array, filename.c_str());
		int inode = el.second;
		strncpy(file_table_array[i].filename, filename_array, filename.size() + 1);
		file_table_array[i].inode_number = inode;
		i++;
	}

	int size = sizeof(file_table_array);
	char ft_buffer[size];
	memset(ft_buffer, 0, size);
	memcpy(ft_buffer, file_table_array, size);
	fseek(current_disk, current_sb.file_table_start *BLOCK_SIZE, SEEK_SET);
	fwrite(ft_buffer, sizeof(char), size, current_disk);

	//SAVING INODE BLOCKS 
	fseek(current_disk, current_sb.inode_start *BLOCK_SIZE, SEEK_SET);
	fwrite(&list_of_inodes, sizeof(struct inode), NO_OF_INODES, current_disk);

	// CLEARING STRUCTURES
	filename_to_fd_map.clear();
	fd_to_filename_map.clear();
	file_table.clear();
	file_mode.clear();
	for (int i = 0; i < NO_OF_INODES; i++)
	{
		list_of_inodes[i].filesize = -1;
		for (int j = 0; j < 12; j++)
		{
			list_of_inodes[i].pointers[j] = -1;
		}
	}

	for (int i = 0; i < FD_LIMIT; i++)
		fd_list[i] = false;
	open_file_count = 0;
}

void enter_file_menu()
{
	while (true)
	{
		int option = get_file_action();
		if (option == 1)
		{
			string filename;
			cout << "Enter filename :";
			cin >> filename;
			create_file(filename);
		}
		else if (option == 2)
		{
			string filename;
			cout << "Enter filename :";
			cin >> filename;
			open_file(filename);
		}
		else if (option == 3)
		{
			int fd;
			cout << "Enter fd number :";
			cin >> fd;
			read_file(fd);
		}
		else if (option == 4)
		{
			int fd;
			cout << "Enter fd number :";
			cin >> fd;
			write_file(fd);
		}
		else if (option == 5)
		{
			int fd;
			cout << "Enter fd number :";
			cin >> fd;
			append_file(fd);
		}
		else if (option == 6)
		{
			int fd;
			cout << "Enter fd number :";
			cin >> fd;
			close_file(fd);
		}
		else if (option == 7)
		{
			string filename;
			cout << "Enter filename :";
			cin >> filename;
			delete_file(filename);
		}
		else if (option == 8)
		{
			list_files();
		}
		else if (option == 9)
		{
			list_open_files();
		}
		else if (option == 10)
		{
			unmount_disk();
			break;
		}
		else if (option == 11)
		{
			display_internal_structres();
		}
		else
		{
			cout << "Invalid option!" << endl;
		}
	}
}

void create_disk()
{
	string diskname;
	cout << "Enter diskname :";
	cin >> diskname;
	if (disk_exists(diskname))
	{
		cout << "Disk with this name already exists!" << endl;
		return;
	}

	FILE *disk = fopen(diskname.c_str(), "wb");
	char file_temp_buffer[BLOCK_SIZE];
	bzero(file_temp_buffer, BLOCK_SIZE);
	for (int i = 0; i < NO_OF_BLOCKS; i++)
		fwrite(file_temp_buffer, sizeof(char), BLOCK_SIZE, disk);

	// WRITING DEFAULT SUPER BLOCK
	superblock sb;
	char buffer[sizeof(sb)];
	for (int i = 0; i < sb.inode_start; i++)
		sb.block_state[i] = false;
	for (int i = sb.inode_start; i < sb.no_of_blocks; i++)
	{
		sb.block_state[i] = true;
	}

	memcpy(buffer, &sb, sizeof(sb));
	fseek(disk, 0, SEEK_SET);
	fwrite(buffer, sizeof(char), sizeof(sb), disk);

	// WRITING INIT FILETABLE
	struct file_inode_entry file_table_array[NO_OF_INODES];
	int size = sizeof(file_table_array);
	char ft_buffer[size];
	memset(ft_buffer, 0, size);
	memcpy(ft_buffer, file_table_array, size);
	fseek(disk, sb.file_table_start *BLOCK_SIZE, SEEK_SET);
	fwrite(ft_buffer, sizeof(char), size, disk);

	// WRITING INIT INODES 
	fseek(disk, sb.inode_start *BLOCK_SIZE, SEEK_SET);
	fwrite(&list_of_inodes, sizeof(struct inode), NO_OF_INODES, disk);
	fclose(disk);
}

void mount_disk()
{
	string diskname;
	cout << "Enter diskname :";
	cin >> diskname;
	if (!disk_exists(diskname))
	{
		cout << "Disk does not exist!";
		return;
	}

	// READ SUPERBLOCK
	current_disk = fopen(diskname.c_str(), "rb+");
	disk_name = diskname;
	char buffer[sizeof(current_sb)];
	fseek(current_disk, 0, SEEK_SET);
	fread(buffer, sizeof(char), sizeof(buffer), current_disk);
	memcpy(&current_sb, buffer, sizeof(current_sb));

	// READ FILETABLE
	fseek(current_disk, current_sb.file_table_start *BLOCK_SIZE, SEEK_SET);
	int size = current_sb.file_table_block_no * BLOCK_SIZE;
	char ft_buffer[size];
	memset(ft_buffer, 0, size);
	fread(ft_buffer, sizeof(char), size, current_disk);
	struct file_inode_entry file_table_array[current_sb.no_of_inode_blocks];
	memcpy(file_table_array, ft_buffer, size);

	for (int i = 0; i < NO_OF_INODES; i++)
	{
		if (file_table_array[i].inode_number != -1)
			file_table.insert({ file_table_array[i].filename, file_table_array[i].inode_number });
	}

	// READ INODES
	fseek(current_disk, current_sb.inode_start *BLOCK_SIZE, SEEK_SET);
	fread(&list_of_inodes, sizeof(struct inode), NO_OF_INODES, current_disk);

	enter_file_menu();
	cout << "Closing disk" << endl;
	fclose(current_disk);
}

int get_disk_action()
{
	int option;
	cout << "=====================================" << endl;
	cout << "DISK MENU" << endl;
	cout << "=====================================" << endl;
	cout << "1 : create_disk" << endl;
	cout << "2 : mount_disk" << endl;
	cout << "3 : exit" << endl << endl;
	cout << "Enter option : ";
	cin >> option;
	while (option < 1 || option > 3)
	{
		cout << "Invalid option!" << endl;
		cout << "=====================================" << endl;
		cout << "DISK MENU" << endl;
		cout << "=====================================" << endl;
		cout << "1 : create_disk" << endl;
		cout << "2 : mount_disk" << endl;
		cout << "3 : exit" << endl;
		cout << "Enter option : ";
		cin >> option;
	}

	return option;
}

int main()
{
	while (true)
	{
		int option = get_disk_action();
		if (option == 1)
		{
			create_disk();
		}
		else if (option == 2)
		{
			mount_disk();
		}
		else if (option == 3)
		{
			break;
		}
		else
		{
			cout << "Invalid option!" << endl;
		}
	}

	return 0;
}
