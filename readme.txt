1. dar make na pasta do projeto
2. na pasta buildroot: sudo ./run.sh
3. modprobe cscan-iosched
4. c-scan > /sys/block/sda/queue/scheduler          #Altera a política de escalonamento, o fcfs é o noop
5. cat /sys/block/sda/queue/scheduler
6. sector_read

Para comparar:
cat /sys/block/sda/stat         #Para a última política usada
