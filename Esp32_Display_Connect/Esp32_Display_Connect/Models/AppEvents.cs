using System.Collections.ObjectModel;

namespace Esp32_Display_Connect.Events;

public sealed record SelectedDeviceChangedEvent(Device? device);
public sealed record TabChangedEvent(int index);
public sealed record DeviceStatusChangedEvent();
public sealed record SettingChangedEvent(string name, float value);


