# Project TODO List

Each section below relates to a phase of the project where we need to stop for manual builds and acceptance testing. 

## Implement basic SPI communication

Status: Complete!

### Description

Create a communication channel between the dom node and the sub node via SPI. The README.md has info on how the hardware is wired. 

The DOM node should:

* wait 5 seconds for the sub node to boot
* connect to the sub
* request the number of wifi nodes from the sub, print the response, repeat every 10 seconds
* retry the connection up to 3 times for initialization and when sub nodes don't respond
* use standard known good SPI configuration, please let me know if it requires me to change my physical wiring

The SUB node should:

* Start scanning for wifi access points
* Store data about the wifi access points in a variable (time offset from boot when first seen, signal strength, ssid, mac address)
* Respond to DOM node when asked how many total APs it has seen
* Continue scanning using a non blocking task

Be sure to log SPI traffic on both the DOM and SUB nodes

## Handle sub node scaling

Status: Complete!

### Description

We need to be able to handle 11 nodes (one for each WiFi channel) at the end of this project, but I currently have two sub nodes connected currently. 

The DOM node should:

* Create an array of pins matching the CS pins listed in the README.md
* Create a shared spi_device_handle to connect to the sub node. We need to only use one SPI handle/connection at a time to reduce memory consumption.
* After waiting 5 seconds for sub nodes to boot, loop through these CS pins one by one. For each pin / sub node:
  * Change cable select to the correct GPIO states for the selected sub node
  * Asking the SUB node how many WiFi APs it can see
* Log the result and which device it came from

The SUB node should:

* Not need any changes to accomodate the DOM node changes

## Implement targeted WiFi scanning

Status: Not started

### Description

Once we have our nodes connected, we need to modify the SPI communication to accomodate more data to be transmitted. In this step we will assign wifi channels from the DOM node to SUB nodes.

The DOM node should:

* Make a reserved list of GPIO pins that corresponds to CS pins for up to 11 nodes
* Have an array ordered by channel popularity in the US to assign to each of these nodes 
* After waiting 5 seconds for sub nodes to boot, go through these CS pins one by one, attempting to assign a WiFi channel to the SUB node on the SPI bus.
* If the device responds we should store it in a list of associated subs for later use

The SUB node should:

* Wait for the DOM node to assign a WiFi channel before setting up the promisious wifi interface
* Accept the channel assignement from the DOM node and give a response. After acknoledging store the channel in a variable 

## Update AP scan to be channel specific 

Status: Not started

### Description

MISSING REQUIREMENT: How often should we send probe request frames?

Built in wifi AP scanning hops channels, we are dividing up this task to multiple SUB nodes in the hopes of not missing and beacon frames.

The SUB node should:

* Take the channel assignment stored in the previous TODO item, change to that WIFI channel, and set up a promiscious interface to listen for beacon frames. There should be a link in RESEARCH.md for this
* Adapt this new function to utilize the previous storage and reporting back to the DOM node.
* Only store nodes in this list once per MAC address, up to 500 total records in the storage buffer, then start removing old records
* Write code to work with the previous version of DOM code if possible, but let me know if changes are needed to the DOM node.


## Add UART GPS to sub nodes 

Status: Not started

### Description

For us to report store our access point list to Wigle.net we will need to add GPS to the SUB nodes. 

MISSING REQUIREMENT: A diagram and explaination of how to connect the UART lines so devices are still debuggable over the USB UART port.

The SUB node should:

* Listen for GPS announcements over UART, in a seperate thread from WiFi scanning or responding to SPI messages
* Store position value and current time in a varialbe
* Add a GPS field to the storage of WiFi APs seen, and update the offset to be an acutal timestamp
* Add other fields as required in the related link in RESEARCH.md 
* Continue to store total number of APs seen, but do not store AP data without a GPS location.

## Add SD card storage to DOM node 

Status: Not started

### Description

The DOM node will store these the data reported by the subs to an SD card

MISSING REQUIREMENT: What is the pinout for connecting to an SD card? I know you can do this without additional hardware

The SUB node:

* Should now send all necessary data for a Wigle.net WiFi access point list (see RESEARCH.md) to the DOM node when nodes are requested
* Should get a confirmation message after data is sent received by the DOM node
* After data is confirmed to be recieved and written, remove the data from the array or data structure storing this data. Add the MAC address to a cache of the last 500 APs reported to the DOM node.
* Check the list of the last 500 APs reported before storing newly discorvered access points.

The DOM node should:

* Ask SUB nodes one at a time for newly discovered APs
* Append recieved data to a .csv file on the attached SD card. Sub nodes will not report data until we get a GPS fix. Store this data in a filename pattern of `wiggle-${timestamp_from_first_row_of_data}.csv`, one file per boot/session 

## Add storage of partial WPA handshakes 

Status: Not started

### Description

It's interesting to me that we may see WPA handshakes with all these SUB nodes in promiscious mode. 

The SUB node should:
* Capture partial WPA handshakes and store these for transport to the DOM

The DOM node should:
* After asking a SUB node for it's newly discovered APs, storing those to a CSV file, and telling the SUB node the records were written, ask the SUB node for handshakes
* The SUB node should respond to these handshakes, please write them to the SD card

## Optimization: GPS cold start 

Status: Not started

### Description

Investigate what it would take to get GPS to start faster by either storing the last known GPS coordinates in the NV RAM or something else