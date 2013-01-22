G21 (mm)
G91 (relative)
G28 X0 Y0 Z0 (physical home)
M104 S230 (temperature)
G1 E10 F250 (flow)
G92 X-100 Y-100 Z0 
G92 E0
G1 Z3 F5000 (prevent diagonal line)
G90 (absolute)
M106 (fan on)
