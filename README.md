## IoT_Edge_Device-ESP32_WiFi
ESP32 WiFi device as a edged IoT device. Accessible via WebAPI  

On running, device creates a open WiFi Access Point(AP) named `IoT_Wifi_config_AP` which can be used to configure WiFi credentials of your network router.Web UI for network configuration is accessible in http://192.168.4.1.

- Once the device is connected to your network, the device is accessible by the WebAPI in the device IP.

- Two controller functions
  - Temperature Sensor
    - The DHT11 Temp & Humidity Sensor Module's out should be connected to ```GPIO4``` pin. 
  - LED read & write
    - Build in LED is used (PIN 2)

### API
- Data fetching endpoints
  -  `POST` request to endpoint: `/fetch` with JSON body
      
      ```json
      {
          "parameter":"led"
      }
      ```
      Here the `parameter` should be `temp` or `led` which are the data should be fetched.
      
  - Updating endpoints
  -  `POST` request to endpoint: `/update` with JSON body
      
      ```json
      {
          "parameter":"led",
          "value":"on"
      }
      ```
      Here the `parameter` should be `led` and `value` should be `on` or `off`.
      
      
      
      
