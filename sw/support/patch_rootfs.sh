#!/bin/sh

# Errata
# ------
#
#  * No tmpfs on /tmp
#
#  * No tools for configuring network interfaces, e.g. the net-tools package
#

sed -i 's/^# \([^$]\+ universe\)/\1/' etc/apt/sources.list

cat <<\EOF >>etc/hostname
parallella-gr-img
EOF

cat <<\EOF >>etc/init/ttyPS0.conf
start on stopped rc or RUNLEVEL=[12345]
stop on runlevel [!12345]

respawn
exec /sbin/getty -L 115200 ttyPS0 vt102
EOF

mkdir -p etc/udev/rules.d
cat <<\EOF >>etc/udev/rules.d/74-parallella-persistent-net.rules
# Always map Parallella network interface to eth0
SUBSYSTEM=="net", ACTION=="add", DEVPATH=="/devices/*amba*/e000b000.eth*/*", DRIVERS=="?*", KERNEL=="eth*", NAME="eth0"
EOF
cat <<\EOF >>etc/udev/rules.d/90-epiphany-device.rules
# Set appropriate permissions on the epiphany device node
KERNEL=="epiphany", MODE="0666" TAG+="systemd", ENV{SYSTEMD_WANTS}+="parallella-thermald@$name.service"
KERNEL=="elink[0-9]*", MODE="0666"
KERNEL=="mesh[0-9]*", MODE="0666" TAG+="systemd", ENV{SYSTEMD_WANTS}+="parallella-thermald@$name.service"
KERNEL=="mesh[0-9]*l[0-9]*", MODE="0666"
EOF

mkdir -p etc/sensors.d
cat <<\EOF >>etc/sensors.d/parallella.conf
chip "iio_hwmon-isa-0000"

    label temp1 "Zynq Temp"
    label in1   "VDD_DSP (Epiphany)"
    ignore in3  # duplicate
    ignore in4  # duplicate
    label in2   "+1.8V"
    ignore in5  # duplicate
    label in6   "+1.35V (DDR)"
    ignore in7  # duplicate
    label in8  "V_ADC"
EOF

cat <<\EOF >>root/ztemp.sh
#!/bin/bash
dir=$(dirname $(grep -rl xadc /sys/bus/iio/devices/*/name))
raw_file=${dir}/in_temp0_raw
offset_file=${dir}/in_temp0_offset
scale_file=${dir}/in_temp0_scale

raw=`cat ${raw_file}`
offset=`cat ${offset_file}`
scale=`cat ${scale_file}`

c_temp=`echo "scale=1;(($raw + $offset) * $scale) / 1000" | bc`
f_temp=`echo "scale=1;(($c_temp * 9) / 5) + 32" | bc`

echo "Zynq Temp: $c_temp C / $f_temp F"
EOF
chmod +x root/ztemp.sh

echo "/dev/ttyPS0" > etc/securetty

#mount -t proc proc proc
#mount --bind /dev dev

# TODO: this assumes the binfmt binary is at /run/binfmt/arm
#		true on my system (povik)
mkdir -p run/binfmt
cp -L /run/binfmt/arm run/binfmt/arm

chroot . /bin/sh <<\EOF
export PATH="/bin:/sbin:/usr/bin:/usr/sbin"

passwd <<\eof
root
root
eof

apt-get -y update
apt-get -y install gnuradio vim udev

apt-get clear
EOF

rm run/binfmt/arm
rm run/binfmt
