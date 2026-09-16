/*using System;
using System.Threading.Tasks;
using Shiny.BluetoothLE;

public class BluetoothService : IBluetoothService
{
    private readonly IBleManager _ble;

    public BluetoothService(IBleManager ble)
    {
        _ble = ble;
    }

    public async Task ScanAsync()
    {
        Console.WriteLine("Starting BLE scan...");

        var subscription = _ble
            .Scan()
            .Subscribe(result =>
            {
                Console.WriteLine(
                    $"Found: {result.Peripheral.Name} " +
                    $"RSSI: {result.Rssi}"
                );
            });

        await Task.Delay(TimeSpan.FromSeconds(10));

        subscription.Dispose();

        Console.WriteLine("BLE scan stopped.");
    }
}*/