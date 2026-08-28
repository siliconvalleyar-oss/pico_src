# Pico Source

Raspberry Pi Pico projects - MIDI Touch Pads and more.

## Projects

### midi/
USB MIDI Controller with analog touch pads.
Reads ADC on GPIO 26, 27, 28 and sends MIDI notes via USB.

## Structure

```
pico_src/
├── midi/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.c
│   │   ├── tusb_config.h
│   │   ├── usb_descriptors.c
│   │   └── usb_descriptors.h
│   ├── docs/
│   │   ├── MANUAL_USO.md
│   │   └── dos/
│   └── pico_sdk_import.cmake
└── README.md
```

## License

MIT
