using System;
using System.Threading.Tasks;

public interface IDeviceConnectionService
{
    event EventHandler<DeviceStatus>? StatusReceived;
    event EventHandler<string>? ConnectionStatusChanged;

    Task ConnectAsync(Device device);
    Task DisconnectAsync();

    void Send(string message);

    bool IsConnected { get; }
}