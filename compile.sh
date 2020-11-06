apt --assume-yes install libgl1-mesa-dev
apt --assume-yes install xorg-dev
gcc satt.c -Ofast -lX11 -lXcursor -pthread -lm -o satt
cp satt /usr/bin/satt
