# !/bin/bash

CURRDIR=$(cd "$(dirname "$0")";pwd) 

if [ -z $1 ]
then
	elf_file=$CURRDIR/../../image/sky39ev200.elf
else
	elf_file=$1
fi

if [ ! -f $elf_file ]
then
	echo -e "\033[1;31m file $elf_file not found.\033[m"
	echo -e "\033[1;31m usage: do_posix_check.sh [elf_file]\033[m"
else
    echo "generating objdump and symbol files...."
    arm-anykav200-linux-uclibcgnueabi-objdump  -h -D -Mreg-names-raw --show-raw-insn $elf_file > $CURRDIR/sky39ev200.txt
    arm-anykav200-linux-uclibcgnueabi-nm -n $elf_file > $CURRDIR/symbols
    
    echo "check invalid posix call ...."
    echo "result:"
    $CURRDIR/find_used_symbols.sh $CURRDIR/sky39ev200.txt $CURRDIR/symbols $CURRDIR/filter
fi
