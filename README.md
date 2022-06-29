### Radio Storm Monitorig Station

Mobile UHF lightning mapping station based on antenna array.
The array could either be mounted stationary or mobile on the car roof.

![Stationary antenna array](./DOC/SRC/img/Stationary_array.jpg "Stationary antenna array on an observatory roof")

![Mobile antenna array](./DOC/SRC/img/mobile_array.jpg "Mobile antenna array on a car roof")

![Mobile receiver mounted](./DOC/SRC/img/car_back_mount.png)

#### Block Schematics

![Station block schamatics](./DOC/SRC/img/RSMS_receiver.png "Overview of interconnection of station components")

Block diagram of UHF radio receiver used in the experiment. The internals of the active antenna mounted on the car roof are depicted in the blue bubbles on the left part of schematics. 

#### Visualization

The device uses [fosphor](https://osmocom.org/projects/sdr/wiki/fosphor) for real-time spectral vizualization.

    sudo apt-get install nvidia-opencl-dev opencl-headers


![Fosphor waterfall for antenna array](./DOC/SRC/img/fosphor_waterfall.png)
