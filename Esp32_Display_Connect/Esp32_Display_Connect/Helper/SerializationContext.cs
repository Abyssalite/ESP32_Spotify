using System.Collections.ObjectModel;
using System.Text.Json.Serialization;

[JsonSourceGenerationOptions(WriteIndented = true)]
[JsonSerializable(typeof(ObservableCollection<Device>))]
internal partial class AppJsonContext : JsonSerializerContext
{
}