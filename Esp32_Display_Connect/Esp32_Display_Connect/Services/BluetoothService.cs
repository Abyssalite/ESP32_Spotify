using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Linux.Bluetooth;
using Linux.Bluetooth.Extensions;

public sealed class BluetoothService : IBluetoothService
{
    private IAdapter1? _adapter;
    private IDevice1? _connectedDevice;

    public event EventHandler<BluetoothDevice>? DeviceDiscovered;

    public async Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        TimeSpan duration,
        CancellationToken cancellationToken = default
    ){
        _adapter ??= await GetAdapterAsync();

        Console.WriteLine($"Using Bluetooth adapter: {_adapter.ObjectPath}");

        // Get devices BlueZ already knows about.
        var devices = await _adapter.GetDevicesAsync();
        var result = new List<BluetoothDevice>();

        foreach (var device in devices)
        {
            var bluetoothDevice = await CreateBluetoothDeviceAsync(device);

            result.Add(bluetoothDevice);
        }

        Console.WriteLine($"Known devices: {result.Count}");

        // Watch for devices appearing during discovery.
        using var watch = await _adapter.WatchDevicesAddedAsync(
            async device => {
                try
                {
                    var bluetoothDevice =
                        await CreateBluetoothDeviceAsync(device);
                        await PrintDeviceDescriptionAsync(device);

                        DeviceDiscovered?.Invoke(this, bluetoothDevice);
                    }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error reading Bluetooth device: {ex}");
                }
            });

        Console.WriteLine($"Starting BLE scan for {duration.TotalSeconds} seconds...");

        await _adapter.StartDiscoveryAsync();

        try
        {
            await Task.Delay(duration, cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // Normal cancellation.
        }
        finally
        {
            await _adapter.StopDiscoveryAsync();

            Console.WriteLine("BLE scan stopped.");
        }

        return result;
    }

    private static async Task PrintDeviceDescriptionAsync(IDevice1 device)
    {
        var properties = await device.GetAllAsync();

        Console.WriteLine($"Device: {properties.Alias}");
        Console.WriteLine($"Address: {properties.Address}");
        Console.WriteLine($"RSSI: {properties.RSSI}");

        if (properties.UUIDs != null)
        {
            Console.WriteLine("UUIDs:");

            foreach (var uuid in properties.UUIDs)
                Console.WriteLine($"  {uuid}");
        }
        Console.WriteLine();

    }

    private async Task<IAdapter1> GetAdapterAsync()
    {
        var adapters = await BlueZManager.GetAdaptersAsync();

        if (adapters.Count == 0)
        {
            throw new InvalidOperationException("No Bluetooth adapters were found.");
        }

        return adapters.First();
    }

    private static async Task<BluetoothDevice> CreateBluetoothDeviceAsync(IDevice1 device)
    {
        var properties = await device.GetAllAsync();

        return new BluetoothDevice
        {
            Id = properties.Address,
            Address = properties.Address,
            Name = properties.Alias,
            Rssi = properties.RSSI,
            ObjectPath = device.ObjectPath.ToString()
        };
    }

    public async Task ConnectAsync(BluetoothDevice device)
    {
        _adapter ??= await GetAdapterAsync();
        var devices = await _adapter.GetDevicesAsync();
        var linuxDevice = devices.FirstOrDefault(d => d.ObjectPath.ToString() == device.ObjectPath);

        if (linuxDevice is null)
        {
            throw new InvalidOperationException($"Bluetooth device '{device.Address}' " + "could not be found.");
        }

        Console.WriteLine($"Connecting to {device.Name ?? device.Address}...");

        await linuxDevice.ConnectAsync();
        _connectedDevice = linuxDevice;

        Console.WriteLine("BLE connected.");
    }

    public async Task DisconnectAsync()
    {
        if (_connectedDevice is null)
            return;

        await _connectedDevice.DisconnectAsync();
        _connectedDevice = null;

        Console.WriteLine("BLE disconnected.");
    }
}