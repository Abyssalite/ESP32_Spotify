
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Avalonia_EventHub;
using Esp32_Display_Connect.Events;

public class Store
{
    private readonly IEventHub _events;
    public required ObservableCollection<Device> DevicesList { get; set; } = new();
    public Device? SelectedDevice;

    public Store(IEventHub events)
    {
        _events = events;
    }

    public void SelectDevice(Device? device)
    {
        if (SelectedDevice == device) return;

        SelectedDevice = device;
        _events.Publish(new SelectedDeviceChangedEvent(SelectedDevice));
    }

    public void StoreUpdateDeviceStatus(DeviceStatus? status)
    {
        if (status == null || SelectedDevice == null) return;

        SelectedDevice.deviceStatus = status;
        _events.Publish(new DeviceStatusChangedEvent());
    }

    public async Task<bool> StoreAddDevice(Device device)
    {
        var deviceName = DevicesList.FirstOrDefault(d => d.Name == device.Name);
        if (deviceName != null) return true;

        DevicesList.Add(device);

        await Helpers.SaveAsync(this);
        return false;
    }
}
