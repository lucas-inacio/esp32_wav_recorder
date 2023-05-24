# Record audio samples to SD card using ESP32 ADC
Record audio with ESP32's ADC peripheral in DMA mode.

## How it works
The ```setup``` function configures the ADC1 peripheral to capture about 44100 samples per second.
The samples are written to a buffer in memory by the ADC through direct memory access.

There is a class (```Wav8BitLoader```) to handle .wav files and load/write them in the SD card.

A separate task (```storeTask```) is created to write samples to the SD card in the .wav file.

The ```loop``` function fills the buffer with samples coming from the ADC.
There are two buffers that are swapped when enough data is available.
When this happens the ```storeTask``` is notified by the ```loop``` and begins to write the SD card.
The ```loop``` can then continue to receive samples on the free buffer.

## Resources
This project is an experiment that uses only the ADC capabilities.
There are other methods to sample audio like I2S, or use the I2S peripheral to acquire from the ADC.
Check the links below to find out how:

- [ESP32 Audio by atomic14](https://github.com/atomic14/esp32_audio)
- [Arduino Audio Tools by pschatzmann](https://github.com/pschatzmann/arduino-audio-tools)

## References
- [ESP-IDF Programming Guide (v4.4.4)](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html)
- [ESP-IDF ADC Example on Github](https://github.com/espressif/esp-idf/tree/release/v4.4/examples/peripherals/adc/dma_read)