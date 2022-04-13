set(include_dirs
	include/arch-common
	include/arch-esp32
#	include/arch-esp32/btle
	include/arch-xmega
	include/btle
	include/btle/siLabsBgApi
	include/collections
	include/commandLineParser
	include/console
	include/consoleMenu
	include/fdLineParser
	include/logger
	include/misc
	include/mqtt
	include/mqtt/messages
	include/mqtt/rpc
	include/net
	include/net/http
	include/peripherals
	include/rpc
	include/runLoop
	include/serial
	include/stateMachine
	include/timeUtils
	)
	
set(srcs
	"src/arch-common/cxa_adcChannel.c"
	"src/arch-common/cxa_batteryCapacityEstimator.c"
	"src/arch-common/cxa_gpio.c"
	"src/arch-common/cxa_gpio_debouncer.c"
	"src/arch-common/cxa_gpio_longPressManager.c"
	"src/arch-common/cxa_gpio_simpleCallback.c"
	"src/arch-common/cxa_i2cMaster.c"
	"src/arch-common/cxa_i2cSlave.c"
	"src/arch-common/cxa_led.c"
	"src/arch-common/cxa_led_gpio.c"
	"src/arch-common/cxa_led_runLoop.c"
	"src/arch-common/cxa_lightSensor.c"
	"src/arch-common/cxa_nvsManager.c"
	"src/arch-common/cxa_rgbLed.c"
	"src/arch-common/cxa_rgbLed_runLoop.c"
	"src/arch-common/cxa_rgbLed_triLed.c"
	"src/arch-common/cxa_tempSensor.c"
	"src/arch-common/cxa_tempSensor_adc.c"
	"src/arch-common/cxa_usart.c"
#	"src/arch-esp32/btle/cxa_esp32_btle_central.c"
#	"src/arch-esp32/btle/cxa_esp32_btle_module.c"
#	"src/arch-esp32/btle/cxa_esp32_btle_peripheral.c"
	"src/arch-esp32/cxa_esp32_criticalSection.c"
	"src/arch-esp32/cxa_esp32_delay.c"
#	"src/arch-esp32/cxa_esp32_ethernetManager.c"
	"src/arch-esp32/cxa_esp32_eventManager.c"
	"src/arch-esp32/cxa_esp32_gpio.c"
	"src/arch-esp32/cxa_esp32_i2cMaster.c"
	"src/arch-esp32/cxa_esp32_mutex.c"
	"src/arch-esp32/cxa_esp32_nvsManager.c"
	"src/arch-esp32/cxa_esp32_random.c"
	"src/arch-esp32/cxa_esp32_reboot.c"
#	"src/arch-esp32/cxa_esp32_sntpClient.c"
	"src/arch-esp32/cxa_esp32_timeBase.c"
	"src/arch-esp32/cxa_esp32_uniqueId.c"
	"src/arch-esp32/cxa_esp32_usart.c"
	"src/arch-esp32/cxa_esp32_wifiManager.c"
	"src/btle/cxa_btle_advPacket.c"
	"src/btle/cxa_btle_central.c"
	"src/btle/cxa_btle_connection.c"
	"src/btle/cxa_btle_connectionManager.c"
	"src/btle/cxa_btle_peripheral.c"
	"src/btle/cxa_btle_uuid.c"
	"src/btle/siLabsBgApi/cxa_siLabsBgApi_btle_central.c"
	"src/btle/siLabsBgApi/cxa_siLabsBgApi_btle_connection.c"
	"src/btle/siLabsBgApi/cxa_siLabsBgApi_btle_peripheral.c"
	"src/btle/siLabsBgApi/cxa_siLabsBgApi_module.c"
	"src/btle/siLabsBgApi/gecko_bglib.c"
	"src/collections/cxa_array.c"
	"src/collections/cxa_fixedByteBuffer.c"
	"src/collections/cxa_fixedFifo.c"
	"src/collections/cxa_linkedField.c"
	"src/commandLineParser/cxa_commandLineParser.c"
	"src/console/cxa_console.c"
	"src/consoleMenu/cxa_consoleMenu.c"
	"src/consoleMenu/cxa_consoleMenu_menu.c"
	"src/fdLineParser/cxa_fdLineParser.c"
	"src/logger/cxa_logger.c"
	"src/misc/cxa_assert.c"
	"src/misc/cxa_eui48.c"
	"src/misc/cxa_numberUtils.c"
	"src/misc/cxa_stringUtils.c"
	"src/misc/cxa_uuid128.c"
	"src/mqtt/cxa_mqtt_client.c"
	"src/mqtt/cxa_mqtt_client_network.c"
	"src/mqtt/cxa_mqtt_connectionManager.c"
	"src/mqtt/cxa_mqtt_messageFactory.c"
	"src/mqtt/cxa_protocolParser_mqtt.c"
	"src/mqtt/messages/cxa_mqtt_message.c"
	"src/mqtt/messages/cxa_mqtt_message_connack.c"
	"src/mqtt/messages/cxa_mqtt_message_connect.c"
	"src/mqtt/messages/cxa_mqtt_message_pingRequest.c"
	"src/mqtt/messages/cxa_mqtt_message_pingResponse.c"
	"src/mqtt/messages/cxa_mqtt_message_publish.c"
	"src/mqtt/messages/cxa_mqtt_message_suback.c"
	"src/mqtt/messages/cxa_mqtt_message_subscribe.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_message.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_node.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_node_bridge.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_node_bridge_multi.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_node_bridge_single.c"
#	"src/mqtt/rpc/cxa_mqtt_rpc_node_root.c"
	"src/net/cxa_network_tcpClient.c"
	"src/net/cxa_network_tcpServer.c"
	"src/net/cxa_network_tcpServer_connectedClient.c"
	"src/net/http/cxa_network_httpClient.c"
#	"src/net/lwipMbedTls/cxa_lwipMbedTls_network_factory.c"
#	"src/net/lwipMbedTls/cxa_lwipMbedTls_network_tcpClient.c"
#	"src/net/lwipMbedTls/cxa_lwipMbedTls_network_tcpServer.c"
#	"src/net/lwipMbedTls/cxa_lwipMbedTls_network_tcpServer_connectedClient.c"
#	"src/net/wolfSslDialSocket/cxa_wolfSslDialSocket_network_factory.c"
#	"src/net/wolfSslDialSocket/cxa_wolfSslDialSocket_network_tcpClient.c"
#	"src/peripherals/cxa_lightSensor_ltr329.c"
#	"src/peripherals/cxa_ltc2942.c"
#	"src/peripherals/cxa_pca9555.c"
#	"src/peripherals/cxa_pca9624.c"
#	"src/peripherals/cxa_rgbLed_pca9624.c"
#	"src/peripherals/cxa_tempRhSensor_si7021.c"
#	"src/peripherals/cxa_tempSensor_si7050.c"
#	"src/peripherals/cxa_ws2812String.c"
#	"src/rpc/cxa_rpc_message.c"
#	"src/rpc/cxa_rpc_messageFactory.c"
#	"src/rpc/cxa_rpc_messageHandler.c"
#	"src/rpc/cxa_rpc_node.c"
#	"src/rpc/cxa_rpc_nodeRemote.c"
	"src/runLoop/cxa_oneShotTimer.c"
	"src/runLoop/cxa_runLoop.c"
	"src/runLoop/cxa_softWatchDog.c"
	"src/serial/cxa_ioStream.c"
	"src/serial/cxa_ioStream_bridge.c"
	"src/serial/cxa_ioStream_loopback.c"
	"src/serial/cxa_ioStream_nullablePassthrough.c"
	"src/serial/cxa_ioStream_peekable.c"
	"src/serial/cxa_ioStream_pipe.c"
	"src/serial/cxa_ioStream_tee.c"
	"src/serial/cxa_protocolParser.c"
	"src/serial/cxa_protocolParser_cleProto.c"
	"src/serial/cxa_protocolParser_crlf.c"
	"src/stateMachine/cxa_stateMachine.c"
	"src/timeUtils/cxa_timeDiff.c"
	)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    PRIV_REQUIRES esp_eth esp_event main nvs_flash tcpip_adapter
                    )
                    
                    