using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Avalonia_EventHub;

public interface IBluetoothService
{
    Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        TimeSpan duration,
        IEventHub _events,
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<BluetoothDevice>> GetKnownDeviceAsync();
    void PrintDeviceDescriptionAsync(BluetoothDevice device);
    Task ConnectAsync(BluetoothDevice device);
    Task SendAsync(string message);
    Task StartReceiveAsync(IEventHub _events);
    Task StopReceiveAsync();
    Task DisconnectAsync();
}