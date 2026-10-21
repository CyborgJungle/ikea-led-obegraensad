#include "plugins/ArtNet.h"

void ArtNetPlugin::setup()
{
  artnet.begin();
  artnet.setArtDmxCallback(onDmxFrame);
  Serial.print("ArtNet server listening at IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(" port: ");
  Serial.println(ART_NET_PORT);
  artnet.setUniverse(1);
  Serial.print("Universe: ");
  Serial.println(artnet.getOutgoing());
}

void ArtNetPlugin::teardown()
{
  // free the UDP port, setup() opens it again
  artnet.stop();
}

void ArtNetPlugin::loop()
{
  artnet.read();
}

const char *ArtNetPlugin::getName() const
{
  return "ArtNet";
}

void ArtNetPlugin::onDmxFrame(uint16_t universe, uint16_t length, uint16_t outgoing, uint8_t *data)
{
  if (universe == 0 || universe == outgoing)
  {
    // the length comes from the packet: never read past the DMX data it carried
    const int channels = std::min<int>(length, MAX_BUFFER_ARTNET - ART_DMX_START);
    for (int i = 0; i < ROWS * COLS; i++)
    {
      const uint8_t value = i < channels ? data[i] : 0;
      Screen.setPixelAtIndex(i, value > 4, value);
    }
  }
}

void ArtNetPlugin::websocketHook(JsonDocument &request)
{
  const char *event = request["event"];

  if (currentStatus == NONE)
  {
    if (!strcmp(event, "artnet"))
    {
      uint16_t universe = request["universe"].as<uint16_t>();
      Serial.print("Changing ArtNet Universe to ");
      Serial.println(universe);
      artnet.setUniverse(universe);
      Serial.print("Current Universe: ");
      Serial.println(artnet.getOutgoing());
    }
  }
}
