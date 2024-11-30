1. dar make na pasta do projeto
2. na pasta buildroot: sudo ./run.sh
3. modprobe cscan-iosched
4. sector_read


modprobe cscan-iosched
echo c-scan > /sys/block/sda/queue/scheduler
cat /sys/block/sda/queue/scheduler
sector_read