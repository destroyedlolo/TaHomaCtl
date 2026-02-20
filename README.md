TaHomaCtl
=====

**TaHomaCtl** is a lightweight command-line interface (CLI) designed to control Somfy TaHoma home automation gateways (Switch, Connexoon, etc.) using the [Local API](https://github.com/Somfy-Developer/Somfy-TaHoma-Developer-Mode).

It is ideal for integration into shell scripts, crontabs, home automation backends or discover your configuration's internals.

# Table of content

- [🚀 Key Features](#---key-features)
  * [What next ?](#what-next--)
- [⚠️ Limitations](#---limitations)
- [🛠 Installation](#---installation)
  * [Prerequisites](#prerequisites)
  * [Compilation](#compilation)
- [📖 Usages](#---usages)
  * [Launching TaHomaCtrl (Shell parameters)](#launching-tahomactrl--shell-parameters-)
  * [TaHomaCtl interactive mode](#tahomactl-interactive-mode)
- [📜 Use cases](#---use-cases)
  * [Discoverying and configuring your TaHoma](#discoverying-and-configuring-your-tahoma)
    + [Scanning your network](#scanning-your-network)
    + [Configure from the shell command line argument](#configure-from-the-shell-command-line-argument)
    + [Using directives](#using-directives)
  * [Querying attached devices](#querying-attached-devices)
  * [Addressing devices](#addressing-devices)
    + [Device's information](#device-s-information)
    + [Querying a device state](#querying-a-device-state)
    + [Steering a device](#steering-a-device)
- [💡 Why TaHomaCtl ?](#---why-tahomactl--)
  * [Integration in my own automation solution](#integration-in-my-own-automation-solution)
  * [Vibe coding testing](#vibe-coding-testing)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>

# 🚀 Key Features

* **Investigate** devices linked to your TaHoma and exposed states and commands.
* **State Monitoring**: Retrieve real-time status and sensor data from your equipment.
* **Control devices**: Send commands to actuators.
* **Low Footprint**: Optimized code, perfect suited for resource limited computers like single-board Raspberry Pi, Orange Pi, BananaPI, etc.

## What next ?

Next version :
* **Steering from CLI arguments** in addition to the actual interactive mode for a better scripting capabilities.

Probably implemented one day (I don't have the use case yet) :
* **Scenario Execution**: Trigger your local scenarios instantly.

Big next step will be to implement TaHoma's compatibility in [Sélené](https://github.com/destroyedlolo/Selene) for a full integration in my own ecosystem.

# ⚠️ Limitations

**TaHomaCtl** is interacting directly with your TaHoma, will not try to interpret results, will not try to secure
dangerous actions : it's only a tool to interact with the Overkiz's public local interface, no more, no less.  
In other words, it has no knowlegde about the devices you're steering.
The benefit is you can control any devices without having to teach TaHomaCtl about it.

As a consequence, it can't interact stuffs managed at Somfy's cloud side (like *Somfy Protect* or *Cloud2Cloud* processes).
As my smart home solution aims to be as local as possible, it's not planned to integrate the "*end user cloud public API*".

> [!WARNING]
> The TaHoma is very slow to respond to some requests at first :
> - mDNS discovery (see discovery section)
> - 1st request handling : if you've got a timeout for the 1st request, retry. As per my own tests, it takes up to 30 seconds to wake up. As soon as the 1st request succeeded, further request will do smoothly.

# 🛠 Installation

## Prerequisites
* A C compiler (GCC/Clang).

3rd party libraries (on Debian derivate, you need the development version `-dev`)
* `libcurl`: For API communication.
* `libjson-c`: For parsing JSON responses.
* `readline` : GNU's readline for command line improvement.

You need the TaHoma's developer mode activated : follow [Overkiz's intruction](https://github.com/Somfy-Developer/Somfy-TaHoma-Developer-Mode).

## Compilation

```
git clone https://github.com/destroyedlolo/TaHomaCtl.git
cd TaHomaCtl
make
```

# 📖 Usages

## Launching TaHomaCtrl (Shell parameters)

Use online help for uptodate list of supported arguments.

```
$ ./TaHomaCtl -h
TaHomaCrl v0.11
	Control your TaHoma box from a command line.
(c) L.Faillie (destroyedlolo) 2025-26

Scripting :
	-f : source provided script
	-N : don't execute ~/.tahomactl at startup

TaHoma's :
	-H : set TaHoma's hostname
	-p : set TaHoma's port
	-k : set bearer token
	-U : don't verify SSL chaine (unsafe mode)

Limiting scanning :
	-4 : resolve Avahi advertisement in IPv4 only
	-6 : resolve Avahi advertisement in IPv6 only

Misc :
	-v : add verbosity
	-V : add even more verbosity
	-t : add tracing
	-d : add some debugging messages
	-h ; display this help
```

* **-U** : don't try to enforce security SSL chaine. Usefull if you haven't imported Overkiz's root CA.
* **-N** : by default, TaHomaCtl will try to source `~/.tahomactl` at startup, which aims to contain TaHoma's connectivity information. This argument prevents it.

## TaHomaCtl interactive mode

Inside the application, you will benefit of GNU's readline features :
- history
- completion (hit TAB key)

An online help is also available :

```
TaHomaCtl > ?
List of known commands
======================

TaHoma's Configuration
----------------------
'TaHoma_host' : [name] set or display TaHoma's host
'TaHoma_address' : [ip] set or display TaHoma's ip address
'TaHoma_port' : [num] set or display TaHoma's port number
'TaHoma_token' : [value] indicate application token
'timeout' : [value] specify API call timeout (seconds)
'status' : Display current connection informations

Scripting
---------
'save_config' : <file> save current configuration to the given file
'script' : <file> execute the file

Interacting with the TaHoma
---------------------------
'scan_TaHoma' : Look for Tahoma's ZeroConf advertising
'scan_Devices' : Query and store attached devices
'Gateway' : Query your gateway own configuration
'Current' : Get action group executions currently running and launched from the local API

Interacting by device's URL
---------------------------
'Device' : [URL] display device "URL" information or the devices list
'States' : <device URL> [state name] query the states of a device
'Command' : <device URL> <command name> [argument] send a command to a device

Interacting by device's name
----------------------------
'NDevice' : [name] display device "name" information or the devices list
'NStates' : <device name> [state name] query the states of a device
'NCommand' : <device name> <command name> [argument] send a command to a device

Verbosity
---------
'verbose' : [on|off|more] Be verbose
'trace' : [on|off|] Trace every commands
'debug' : [on|off] enable debug messages

Miscs
-----
'#' : Comment, ignored line
'?' : List available commands
'history' : List command line history
'Quit' : See you
```

# 📜 Use cases

## Discoverying and configuring your TaHoma

### Scanning your network
The first step is to configure TaHomaCtl about your gateway using `scan_TaHoma`.  
`save_config` will save found information in the given file.

```
$ ./TaHomaCtl -Uv
*W* SSL chaine not enforced (unsafe mode)
TaHomaCtl > scan_TaHoma 
*I* Service 'gateway-xxxx-xxxx-xxxx' of type '_kizboxdev._tcp' in domain 'local':
TaHomaCtl > save_config .tahomactl
TaHomaCtl > Quit
```

> [!TIP]
> Be patient : the TaHoma doesn't advertise often and react slowly to mDNS response.

For next utilization, you can avoid network scanning by providing yourself your gateway information as below :

### Configure from the shell command line argument

[See above](#launching-tahomactrl--shell-parameters-)

### Using directives

Inline commands are `TaHoma_host`, `TaHoma_address`, `TaHoma_port`.  
Don't forget, we did use `save_config` to create a script containing all the needed :smirk:.
If you saved it as "**.tahomactl**" in your home directory, it will be automatically loaded at startup.

> [!CAUTION]
> Don't forget to indicate the bearer code using `TaHoma_token`

While `status` will give you the current connection setting,
`Gateway` will query your TaHoma about its own configuration.

```
$ ./TaHomaCtl -U
TaHomaCtl > status
*I* Connection :
	Tahoma's host : gateway-xxxx-xxxx-xxxx.local
	Tahoma's IP : 192.168.0.30
	Tahoma's port : 8443
	Token : set
	SSL chaine : not checked (unsafe)
*I* 0 Stored device 
TaHomaCtl > Gateway 
gatewayId : xxxx-xxxx-xxxx
Connected : OK
protocolVersion : 2026.1.3-3
TaHomaCtl > 
```

## Querying attached devices

`scan_Devices` will read from your TaHoma attached devices.

```
$ ./TaHomaCtl -Uv
*W* SSL chaine not enforced (unsafe mode)
TaHomaCtl > scan_Devices 
*I* 15 devices
```

> [!CAUTION]
> This request is very resource-intensive for the TaHoma, especially if you have many connected devices.
> It is therefore advisable to use it as infrequently as possible, generally only once at startup.

`Devices` will display stored devices.

```
TaHomaCtl > Device
test_air : zigbee://xxxx-xxxx-xxxx/58849/1#1
test_air : zigbee://xxxx-xxxx-xxxx/58849/1#2
test_air : zigbee://xxxx-xxxx-xxxx/58849/3
test_air : zigbee://xxxx-xxxx-xxxx/58849/1#4
Deco : io://xxxx-xxxx-xxxx/5335270
Porte_Chat : rts://xxxx-xxxx-xxxx/16774417
IO_(10069463) : io://xxxx-xxxx-xxxx/10069463
test_air : zigbee://xxxx-xxxx-xxxx/58849/0
ZIGBEE_(0/0) : zigbee://xxxx-xxxx-xxxx/0/0
Boiboite : internal://xxxx-xxxx-xxxx/pod/0
INTERNAL_(wifi/0) : internal://xxxx-xxxx-xxxx/wifi/0
test_air : zigbee://xxxx-xxxx-xxxx/58849/1#3
ZIGBEE_(0/242) : zigbee://xxxx-xxxx-xxxx/0/242
ZIGBEE_(0/1) : zigbee://xxxx-xxxx-xxxx/0/1
ZIGBEE_(65535) : zigbee://xxxx-xxxx-xxxx/65535
```

Where you can see for each of them :
- the **name** as configured in **TaHoma by Somfy** application
- the **URL**

As you can see above, some sensors may be discovered multiple times :
here, **test_air** is a Zigbee multisensor, and each of them is seen as a different device with the same name.
Only the URL can discriminate them.

## Addressing devices

Devices can be identified by their names or their URLs using dedicated commands as described below.

### Device's information

`Device` and `NDevice` are displaying exposed commands and states of a specific device.

```
TaHomaCtl > Device internal://xxxx-xxxx-xxxx/pod/0
Boiboite : internal://xxxx-xxxx-xxxx/pod/0
	Commands
		activateCalendar (0 arg)
		setCountryCode (1 arg)
		update (0 arg)
		deactivateCalendar (0 arg)
		refreshPodMode (0 arg)
		refreshUpdateStatus (0 arg)
		setCalendar (1 arg)
		getName (0 arg)
		setLightingLedPodMode (1 arg)
		setPodLedOff (0 arg)
		setPodLedOn (0 arg)
	States
		internal:Button3State
		core:ConnectivityState
		internal:LightingLedPodModeState
		internal:Button1State
		internal:Button2State
		core:CountryCodeState
		core:LocalAccessProofState
		core:LocalIPv4AddressState
		core:NameState
TaHomaCtl > NDevice Deco
Deco : io://xxxx-xxxx-xxxx/5335270
	Commands
		toggle (0 arg)
		resetLockLevels (0 arg)
		advancedRefresh (1 arg)
		setConfigState (1 arg)
		unpairAllOneWayControllers (0 arg)
		onWithTimer (1 arg)
		wink (1 arg)
		addLockLevel (1 arg)
		removeLockLevel (1 arg)
		setOnOff (1 arg)
		off (0 arg)
		on (0 arg)
		setName (1 arg)
		identify (0 arg)
		getName (0 arg)
		delayedStopIdentify (1 arg)
		pairOneWayController (1 arg)
		startIdentify (0 arg)
		stopIdentify (0 arg)
		unpairOneWayController (1 arg)
	States
		core:OnOffState
		io:PriorityLockOriginatorState
		io:PriorityLockLevelState
		core:PriorityLockTimerState
		core:RSSILevelState
		core:DiscreteRSSILevelState
		core:NameState
		core:CommandLockLevelsState
		core:StatusState
```

### Querying a device state

```
TaHomaCtl > NStates Deco 
	core:StatusState : "available"
	core:CommandLockLevelsState : [Array]
	core:DiscreteRSSILevelState : "normal"
	core:RSSILevelState : "54"
	core:OnOffState : "off"
	core:PriorityLockTimerState : "0"
	io:PriorityLockOriginatorState : "unknown"
	core:NameState : "Deco"
```

It's also possible to query a single state. In such case, only the value is returned.

```
TaHomaCtl > NStates Deco core:OnOffState
"off"
```

or by its URL

```
TaHomaCtl > States zigbee://2095-0445-1705/58849/1#1 core:TemperatureState
18.300000
```

### Steering a device

`Command` and `NCommand` can send an order to a given device, potentially with parameters.

```
TaHomaCtl > NCommand IO_(10069463) discoverSomfyUnsetActuators
*I* Successful
```

> [!CAUTION]
> The number of argument is not verified and more their types (as not provided).
> The TaHoma box will reject incorrect commands.

Actually running orders (that are not too fast to be seen) can be listed using **Current** command.

```
TaHomaCtl > Command IO_(10069463) discoverSomfyUnsetActuators
*I* Successful
TaHomaCtl > Current 
*I* 1 Execution(s)
*I* id: e061b077-7330-4fe9-8446-48b11c2ddbe2
	description : "Execution : TaHomaCtl"
	owner: local	state: IN_PROGRESS	type: MANUAL_CONTROL
	Started on Sat Feb 14 20:05:50 2026
		Device : IO_(10069463) 'io://xxxx-xxxx-xxxx/10069463'
		[
			name :
				"discoverSomfyUnsetActuators"
			parameters :
				[
				]
		]
TaHomaCtl > 
```

# 💡 Why TaHomaCtl ?

## Integration in my own automation solution

My own smart home solution is quite efficient and complete ([Marcel](https://github.com/destroyedlolo/Marcel), [Majordome](https://github.com/destroyedlolo/Majordome), ...), no need to replace anything. But I was wondering how I can integrate this nice box to my own ecosystem, as example to steer **IO-homecontrol** devices.

TaHomaCtl was initially made as a Proof of Concept (PoC) before integrating something in Marcel (or as an autonomous daemon).

> [!NOTE]
> Yes, I'm proudly part of Somfy/Overkiz, but this code doesn't contain any internals' and will not to avoid any interest conflict : it was built only from publicly available information and my own testing.<br>
> No, don't ask me about anything not made public, I'll not reply.

## Vibe coding testing

**TaHomaCtl** uses some technologies I hadn't coded before, like mDNS advertising. This small project was a good candidate to test AI companions (ChatGPT and Gemini) and vibe coding. AI generated code, sometime corrected, can be found in TestCodes. The result is quite mixed :

- mDNS : the outcome is poor, sometime bad ! ChatGPT stays on its own mistakes (non-existent functions, stupid assumptions, wrong strategies, ...). Then I ask Gemini to correct and got some improvements. But, all in all, the result is heavy, not flexible and doesn't suite my quality standard, by far.

- GNU readline : generated code was better ... but still not functional until manual corrections.

Well, the AI saves me some time to find technical information, replacing google researches. But it is far far away to build from scratch something.
