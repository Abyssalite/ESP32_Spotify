namespace Esp32_Display_Connect.Events;

public sealed record SelectedDeviceChangedEvent(Device? device);

public sealed record DeviceStatusChangedEvent();
public sealed record SettingChangedEvent(string name, float value);

public sealed record StatusReceivedEvent(DeviceStatus deviceStatus);
public sealed record ConnectionStatusChangedEvent(string connectionStatus);

public sealed record BluetoothDiscoveredEvent(BluetoothDevice device);
