cd $1
mkdir makedir
cd makedir
cpu_num=`cat /proc/cpuinfo| grep "processor"| wc -l`
cmake .. -DCMAKE_INSTALL_PREFIX=$2
make install -j$cpu_num

sudo sed -i '/DYNINSTAPI_RT_LIB/d' /etc/profile
sudo echo "DYNINSTAPI_RT_LIB=$2/lib/libdyninstAPI_RT.so"
