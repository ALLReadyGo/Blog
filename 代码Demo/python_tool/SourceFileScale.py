#!/usr/bin/python3
import os
import sys

'''
@brief:
    用于获取代码的规模（代码目录包含的文件数、代码的行数）
@example:
    SourceFileScale.py filename        # 获取某一文件的信息（文件大小，文件行数）
    SourceFileScale.py sourceDir       # 获取源代码目录的信息，能够利用suffix来自动过滤掉非源码文件
'''

suffix = ['.cc', '.h', '.py']

def linesOfFile(url):
    with open(url, 'r') as fp:
        lines = len(fp.readlines())
    return lines


def infoOfSingleSourceFile(url):
    if not '.' + url.split('.')[-1] in suffix:
        print("input file not a recongnized file")
        return
    file_name = url.split('/')[-1]
    file_size = os.path.getsize(url)
    file_line = linesOfFile(url)
    print(f'file_name:    {file_name}')
    print(f'file_size:    {file_size}')
    print(f'file_lines:   {file_lines}')

def infoOfSourceDir(dir):
    various_file_nums = {key: 0 for key in suffix}
    files_total_lines = 0

    dir_queue = [dir]
    while len(dir_queue) != 0:
        for path in os.listdir(dir_queue[0]):
            path = os.path.join(dir_queue[0], path)
            if os.path.isdir(path):
                dir_queue.append(path)
            elif os.path.isfile(path):
                file_suffix = '.' + path.split('.')[-1]
                if file_suffix in suffix:
                    various_file_nums[file_suffix] += 1
                    files_total_lines += linesOfFile(path)
        dir_queue.pop(0)
    for key, value in various_file_nums.items():
        print(f'{key}\t{value}')
    print(f'total lines:  {files_total_lines}')

if __name__ == '__main__':
    if len(sys.argv) < 1:
        print("input file or dirname as arguments(only one arguemnt)")
        exit(1)
    url = sys.argv[1]
    if os.path.isfile(url):
        infoOfSingleSourceFile(url)
    elif os.path.isdir(url):
        infoOfSourceDir(url)