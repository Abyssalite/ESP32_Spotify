using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Avalonia_EventHub;
using Linux.Bluetooth;
using Linux.Bluetooth.Extensions;
using Esp32_Display_Connect.Events;

public sealed class BluetoothService : IBluetoothService
{
    private IAdapter1? _adapter;
    private IDevice1? _connectedDevice;

    public async Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        TimeSpan duration,
        IEventHub _events,
        CancellationToken cancellationToken = default
    ){
        _adapter ??= await GetAdapterAsync();
        var result = new List<BluetoothDevice>();

        Console.WriteLine($"Using Bluetooth adapter: {_adapter.ObjectPath}");
        
        // Watch for devices appearing during discovery.
        using var watch = await _adapter.WatchDevicesAddedAsync(
            async device => {
                try
                {
                    var bluetoothDevice = await CreateBluetoothDeviceAsync(device);
                    result.Add(bluetoothDevice);

                    _events.Publish(new BluetoothDiscoveredEvent(bluetoothDevice));

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

    public async Task<IReadOnlyList<BluetoothDevice>> GetKnownDeviceAsync()
    {
        _adapter ??= await GetAdapterAsync();

        var devices = await _adapter.GetDevicesAsync(); 
        var result = new List<BluetoothDevice>(); 
        
        Console.WriteLine($"Using Bluetooth adapter: {_adapter.ObjectPath}");

        foreach (var device in devices) 
        { 
            var bluetoothDevice = await CreateBluetoothDeviceAsync(device); 
            result.Add(bluetoothDevice); 
        } 

        Console.WriteLine($"Known devices: {result.Count}");

        return result;
    }

    public void PrintDeviceDescriptionAsync(BluetoothDevice device)
    {
        Console.WriteLine($"Device: {device.Name}");
        Console.WriteLine($"Address: {device.Address}");
        Console.WriteLine($"RSSI: {device.Rssi}");

        if (device.Uuids != null)
        {
            Console.WriteLine("UUIDs:");

            foreach (var uuid in device.Uuids)
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
            Uuids = properties.UUIDs,
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