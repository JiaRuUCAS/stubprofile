cd $1
mkdir makedir
cd makedir
cpu_num=`cat /proc/cpuinfo| grep "processor"| wc -l`
cmake .. -DCMAKE_INSTALL_PREFIX=$2
make install -j$cpu_num
