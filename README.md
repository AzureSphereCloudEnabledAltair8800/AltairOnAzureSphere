Welcome to the _AI and Cloud powered Altair 8800_ emulator repo. If you're interested in [retrocomputing](https://en.wikipedia.org/wiki/Retrocomputing), software development, AI, cloud services, and climate monitoring you've arrived at the right repo.

The [Altair 8800](https://en.wikipedia.org/wiki/Altair_8800?azure-portal=true) kick-started the personal computer revolution. Microsoft's first product was [Altair BASIC](https://en.wikipedia.org/wiki/Altair_BASIC?azure-portal=true) written for the Altair 8800 by Bill Gates and Paul Allen. At the time, Altair BASIC was a huge step forward as it allowed people to write programs using a high-level programming language.

The Altair project can run standalone and is a fantastic safe way to explore machine-level programming, Intel 8080 Assembly programming, along with C and BASIC development.

Optionally, the project integrates free weather and pollution cloud services from [Open Weather Map](http://openweathermap.org), along with [Azure IoT Central](https://azure.microsoft.com/en-au/services/iot-central/), and [Azure Anomaly Detection Cognitive Service](https://azure.microsoft.com/services/cognitive-services/anomaly-detector/). The following reports were generated from data published by the Altair emulator using data sourced from Open Weather Map.

| IoT Central Sydney pollution report | Azure Anomaly Detection report|
|------|-----|
| ![The images shows pollution report for Sydney](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800_V2/wiki/media/iot_central_pollution_report.png) | ![The following images shows temperature based anomalies](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800_V2/wiki/media/univariate-anomalies.png) |

## Documentation

The project documentation is maintained on the repo [Wiki](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800_V2/wiki).

## Supported devices

The Altair emulator is supported on Azure Sphere devices from Avnet and Seeed Studio. If an Azure Sphere device is paired with the [Altair front panel kit](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800.Hardware) or the [Mikroe Altair Retro Click](https://www.mikroe.com/blog/8800-retro-click), the Altair address and data bus activity is displayed.

| Azure Sphere with the Altair front panel kit | MikroE Retro Click |
|--|--|
| ![The gif shows the Altair on Azure Sphere with the Altair front panel](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800_V2/wiki/media/altair_on_sphere.gif) | ![The gif shows the address and data bus LEDs in action](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereAltair8800_V2/wiki/media/avnet_retro_click.gif) |
