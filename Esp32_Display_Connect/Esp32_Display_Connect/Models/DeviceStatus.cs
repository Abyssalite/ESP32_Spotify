public sealed class DeviceStatus
{
    public int Fan1Speed { get; set; }
    public int Fan2Speed { get; set; }
    public int TecPower { get; set; }
    public float SetTemp { get; set; }
    public float CurrentTemp { get; set; }
    public float CurrentHumidity { get; set; }
    public float ThermTemp { get; set; }
    public int Mode { get; set; }
    public int ModeIndex { get; set; }
    public int NumberMode { get; set; }

}