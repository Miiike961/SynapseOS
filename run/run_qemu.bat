:: Qemu config

::qemu-system-x86_64 -m 32 -kernel bin/kernel.elf -monitor stdio -serial file:Qemu_log.txt -no-reboot 
qemu-system-i386 -m 32 -boot d -cdrom file:../SynapseOS.iso -fda file:A.img -fdb file:B.img -monitor stdio -serial file:Qemu_log.txt -no-reboot 

pause