public sealed class BluetoothDevice
{
    public required string Id { get; init; }

    public required string Address { get; init; }

    public string? Name { get; init; }

    public short? Rssi { get; init; }

    // Linux.Bluetooth object path.
    public string? ObjectPath { get; init; }

    public override string ToString()
        => $"{Name ?? "<unknown>"} ({Address})";
}