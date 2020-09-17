# Měřič spotřeby elektřiny s ESP32 a INA-219 (ina219-esp32-power_meter)

Aplikace pro měřič spotřeby s ESP32 (LilyGO TTGO T-Display) a obvodem INA-219.

Více informací zde: https://pebrou.wordpress.com/2020/08/29/stavime-chytry-meric-spotreby-s-esp32-a-modulem-ina219/

- Vlastní základní (nepřipojená na internet) aplikace je v ina219_app_g1/
- V ina219_app_wifi_g2 je verze, která odesílá data na internet. 
- V iic_scanner/ je jednoduchý scanner I2C sběrnice pro vyhledání adresy modulu INA219 , pokud není na standardní adrese.
- V schema_a_deska/ najdete schema zapojení. Návrh desky tam teď není.
