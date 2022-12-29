#pragma once

class Child {
  public:
    explicit Child(const char* baseuid, uint8_t id, const char* lbl, rl_device_t st, rl_data_t dt);
    uint8_t getId();
    char* getLabel();
    rl_device_t getSensorType();
    rl_data_t getDataType();
    void doPacketForHA(rl_packet_t p);
    HABaseDeviceType* getHADevice();
    void switchToLoRa(uint8_t address, bool state);
  protected:
    uint8_t _id;
    char* _label;
    char* _HAuid;
    rl_device_t _sensorType;
    rl_data_t _dataType;
    HABaseDeviceType* _HAdevice;
};

class Device {
  public:
    explicit Device(uint8_t address, const char* name);
    Child* addChild(Child* ch);
    uint8_t getAddress();
    char* getName();
    int getNbChild();
    Child* getChild(int n);
    Child* getChildById(uint8_t id);
  protected:
    uint8_t _address;
    char* _name;
    Child** _childList;
    int _nbChild;
};

class HubDev {
  public:
    explicit HubDev();
    uint8_t getNbDevice();
    Device* addDevice(Device* dev);
    Device* getDevice(int n);
    Child* getChildById(uint8_t address, uint8_t id);
  protected:
    Device** _deviceList;
    int _nbDevice;
};

HubDev hub;

//************************************************
void onSwitchCommand(bool state, HASwitch* HAsender)
{
  /*
    if (sender == &switch1) {
        // the switch1 has been toggled
        // state == true means ON state
    } else if (sender == &switch2) {
        // the switch2 has been toggled
        // state == true means ON state
    }
    sender->setState(state); // report state back to the Home Assistant
  */
  DEBUGln("receive switch");
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    for (uint8_t c = 0; c < dev->getNbChild(); c++)
    {
      Child* ch = dev->getChild(c);
      if (ch && (ch->getHADevice() == HAsender))
      {
        ch->switchToLoRa(dev->getAddress(), state);
      }
    }
  }
}

//************************************************
HubDev::HubDev()
{
  _nbDevice = 0;
  _deviceList = nullptr;
}

uint8_t HubDev::getNbDevice()
{
  return _nbDevice;
}

Device* HubDev::getDevice(int n)
{
  if (n < _nbDevice)
  {
    return _deviceList[n];
  }
  return nullptr;
}

Device* HubDev::addDevice(Device * dev)
{
  _deviceList = (Device**)realloc(_deviceList, (_nbDevice + 1) * sizeof(Device*));
  if (_deviceList == nullptr)
  {
    DEBUGln("erreur realloc");
    return nullptr;
  }
  _deviceList[_nbDevice] = dev;
  _nbDevice++;
  return dev;
}

Child* HubDev::getChildById(uint8_t address, uint8_t id)
{
  for (uint8_t d = 0; d < _nbDevice; d++)
  {
    Device* dev = _deviceList[d];
    if (dev->getAddress() == address)
    {
      return dev->getChildById(id);
    }
  }
  return nullptr;
}

//************************************************
Device::Device(uint8_t address, const char* name)
{
  _name = (char*)malloc(strlen(name) + 1);
  strcpy(_name, name);
  _name[strlen(name)] = 0;
  _address = address;
  _nbChild = 0;
  _childList = nullptr;
}

Child* Device::addChild(Child * ch)
{
  _childList = (Child**)realloc(_childList, (_nbChild + 1) * sizeof(Child*));
  if (_childList == nullptr) {
    DEBUGln("defaut realloc");
    return nullptr;
  }
  _childList[_nbChild] = ch;
  _nbChild++;
  return ch;
}

uint8_t Device::getAddress()
{
  return _address;
}

char* Device::getName()
{
  return _name;
}

int Device::getNbChild()
{
  return  _nbChild;
}

Child* Device::getChild(int n)
{
  if (n < _nbChild)
  {
    return _childList[n];
  }
  return nullptr;
}

Child* Device::getChildById(uint8_t id)
{
  for (uint8_t c = 0; c < _nbChild; c++)
  {
    if (_childList[c]->getId() == id)
    {
      return _childList[c];
    }
  }
  return nullptr;
}

//************************************************
Child::Child(const char* baseuid, uint8_t id, const char* lbl, rl_device_t st, rl_data_t dt)
{
  _label = (char*)malloc(strlen(lbl) + 1);
  strcpy(_label, lbl);
  _label[strlen(lbl)] = 0;
  _HAuid = (char*)malloc(strlen(baseuid) + strlen(_label) + 2);
  strcpy(_HAuid, baseuid);
  strcat(_HAuid, "_");
  strcat(_HAuid, _label);
  _id = id;
  _sensorType = st;
  _dataType = dt;
  _HAdevice = nullptr;
  switch (st) {
    case S_BINARYSENSOR:
      _HAdevice = new HABinarySensor(_HAuid);
      break;
    case S_NUMERICSENSOR:
      _HAdevice = new HASensorNumber(_HAuid);
      break;
    case S_SWITCH:
      _HAdevice = new HASwitch(_HAuid);
      ((HASwitch*)_HAdevice)->onCommand(onSwitchCommand);
      break;
    case S_LIGHT:
      _HAdevice = new HALight(_HAuid);
      break;
    case S_COVER:
      _HAdevice = new HACover(_HAuid);
      break;
    case S_FAN:
      //TODO
      break;
    case S_HVAC:
      // TODO
      break;
    case S_SELECT:
      // TODO
      break;
    case S_TRIGGER:
      _HAdevice = new HADeviceTrigger(HADeviceTrigger::ButtonShortPressType, _HAuid);
      break;
    case S_CUSTOM:
      // TODO
      break;
    case S_TAG:
      _HAdevice = new HATagScanner(_HAuid);
      break;
  }
  if (_HAdevice)
  {
    _HAdevice->setName(_HAuid);
  }
}

uint8_t Child::getId()
{
  return _id;
}

char* Child::getLabel()
{
  return _label;
}

rl_device_t Child::getSensorType()
{
  return _sensorType;
}

rl_data_t Child::getDataType()
{
  return _dataType;
}

HABaseDeviceType* Child::getHADevice()
{
  return _HAdevice;
}

void Child::switchToLoRa(uint8_t address, bool state)
{
  DEBUGf("switch to lora %s\n", _label);
  RLcomm.publishNum(address, 0, _id, _sensorType, state);
}

void Child::doPacketForHA(rl_packet_t p)
{
  byte sender = p.senderID;
  byte child = p.childID;
  DEBUGf("Do packet from %d / %d : %s (%d)\n", sender, child, _HAuid, p.data.num.value);
  rl_device_t st = (rl_device_t)((p.sensordataType >> 3) & 0x1F);
  rl_data_t dt;
  float fval;
  String sval;
  char tag[9] = {0};
  switch (st)
  {
    case S_BINARYSENSOR:
      ((HABinarySensor*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_NUMERICSENSOR:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      if (dt == V_FLOAT)
      {
        fval = p.data.num.value;
        if (p.data.num.divider != 0)
        {
          fval = (float)p.data.num.value / (float)p.data.num.divider;
        }
        DEBUGf("send float %f\n", fval);
        ((HASensorNumber*)_HAdevice)->setPrecision(p.data.num.precision % 4); // %4 for precision 0 to 3
        ((HASensorNumber*)_HAdevice)->setValue(fval);
      }
      else
      {
        DEBUGf("send num %d\n", p.data.num.value);
        ((HASensorNumber*)_HAdevice)->setValue(p.data.num.value);
      }
      break;
    case S_SWITCH:
      ((HASwitch*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_LIGHT:
      ((HALight*)_HAdevice)->setState(p.data.num.value > 0);
      break;
    case S_COVER:
      ((HACover*)_HAdevice)->setState((HACover::CoverState)p.data.num.value);
      break;
    case S_FAN:
      // TODO
      break;
    case S_HVAC:
      // TODO
      break;
    case S_SELECT:
      // TODO
      break;
    case S_TRIGGER:
      dt = (rl_data_t)(p.sensordataType & 0x07);
      sval = String(p.data.num.value);
      if (dt == V_TEXT)
      {
        sval = String(p.data.text);
      }
      ((HADeviceTrigger*)_HAdevice)->trigger(sval.c_str());
      break;
    case S_CUSTOM:
      // TODO
      break;
    case S_TAG:
      HAUtils::byteArrayToStr(tag, p.data.rawByte, 4);
      ((HATagScanner*)_HAdevice)->tagScanned(tag);
      break;
  }
}

// ******************************************
