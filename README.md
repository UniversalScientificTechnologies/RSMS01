### Radio Storm Monitoring Station

![Lightning signal receiver RSMS01B](./DOC/SRC/img/RSMS01B_receiver.jpg "UHF lightnig signal receiver RSMS01B")

Mobile UHF lightning mapping station based on an antenna array. VLF version exists as [RSMS02](https://github.com/UniversalScientificTechnologies/RSMS02).
The array could either be stationary or mobile, mounted on the car roof.

![Stationary antenna array](./DOC/SRC/img/Stationary_array.jpg "Stationary antenna array on an observatory roof")

![Mobile antenna array](./DOC/SRC/img/mobile_array.jpg "Mobile antenna array on a car roof")

![Mobile receiver mounted](./DOC/SRC/img/car_back_mount.png)

#### Block Schematics

![Station block schamatics](./DOC/SRC/img/RSMS_receiver.png "Overview of interconnection of station components")

Block diagram of UHF radio receiver used in the experiment. The internals of the active antenna mounted on the car roof are depicted in the blue bubbles on the left part of schematics.

#### Visualization

The device uses [fosphor](https://osmocom.org/projects/sdr/wiki/fosphor) for real-time spectral visualization.

    sudo apt-get install nvidia-opencl-dev opencl-headers


![Fosphor waterfall for antenna array](./DOC/SRC/img/fosphor_waterfall.png)

#### Relevant scientific publications

  * [In situ ground-based mobile measurement of lightning events above central Europe](https://amt.copernicus.org/articles/16/547/2023/)
