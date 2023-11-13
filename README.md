# Climate Portable LoRa
### Proyek Tugas Akhir Magang [Divisi Elektrikal]
***

Alat ini dapat memungkinkan beberapa NODE mengirimkan bacaan sensor pada MASTER dengan komunikasi Wireless [LoRa RFM95 - 915Mhz]

MASTER atau GATEWAY dapat mengirimkan nilai sensor dari NODE kepada MQTT melalui koneksi WiFi

MQTT akan dipanggil menggunakan NODE-RED untuk ditampilkan pada DASHBOARD

<a href="https://ibb.co/QjP9yH6"><img src="https://i.ibb.co/zHXsq2S/Sampel.png" alt="Sampel" border="0"></a>

> Berikut prinsip kerja alat
> 1. Slave 1 dan Slave 2 mengirimkan nilai sensor kepada master secara bergantian menggunakan komunikasi LoRa 
> 2. Master menerima nilai sensor dari masing-masing slave
> 3. Master meneruskan mengirimkan nilai bacaan sensor ke topic MQTT secara bergantian dengan koneksi internet melalui WiFi
