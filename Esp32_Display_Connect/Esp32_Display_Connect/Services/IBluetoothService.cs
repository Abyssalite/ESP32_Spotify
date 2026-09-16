using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

public interface IBluetoothService
{
    event EventHandler<BluetoothDevice>? DeviceDiscovered;

    Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        TimeSpan duration,
        CancellationToken cancellationToken = default);

    Task ConnectAsync(BluetoothDevice device);

    Task DisconnectAsync();
}