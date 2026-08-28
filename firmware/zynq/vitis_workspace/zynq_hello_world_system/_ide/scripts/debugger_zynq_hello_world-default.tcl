# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/zynq_hello_world_system/_ide/scripts/debugger_zynq_hello_world-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/zynq_hello_world_system/_ide/scripts/debugger_zynq_hello_world-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/target_control_zynq_platform/export/target_control_zynq_platform/hw/target_control_zynq_hw.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/zynq_hello_world/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/zynq_hello_world/Debug/zynq_hello_world.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
