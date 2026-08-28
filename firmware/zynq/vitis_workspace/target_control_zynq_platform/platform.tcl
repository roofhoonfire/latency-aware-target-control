# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/target_control_zynq_platform/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace/target_control_zynq_platform/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {target_control_zynq_platform}\
-hw {/home/user/Desktop/latency-aware-target-control/firmware/zynq/target_control_zynq_hw/target_control_zynq_hw.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -out {/home/user/Desktop/latency-aware-target-control/firmware/zynq/vitis_workspace}

platform write
platform generate -domains 
platform active {target_control_zynq_platform}
platform generate
domain create -name {freertos_domain} -os {freertos} -proc {ps7_cortexa9_0} -arch {32-bit} -display-name {freertos_domain} -desc {} -runtime {cpp}
platform generate -domains 
platform write
domain -report -json
platform generate -domains freertos_domain 
