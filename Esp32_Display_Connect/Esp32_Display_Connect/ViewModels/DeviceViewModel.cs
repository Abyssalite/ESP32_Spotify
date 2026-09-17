using System.Threading;
using System.Threading.Tasks;
using Avalonia_EventHub;
using Avalonia_Navigation;
using Esp32_Display_Connect.Events;

namespace Esp32_Display_Connect.ViewModels;

public partial class DeviceViewModel : ViewModelBase, IHandleBackNavigation
{    
    private CancellationTokenSource? _delayToken;

    private readonly ITabView _tabview;
    public ITabView TabView => _tabview;
    private readonly IDeviceConnectionService _connection;

    public Device? SelectedDevice { get; }
    public string? Status { set; get; }

    public DeviceViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        ITabView tabs,
        IDeviceConnectionService connection
    ):base(store, navigator, events)
    {        
        _connection = connection;
        _tabview = tabs;
        SelectedDevice = _store.SelectedDevice;
        if (SelectedDevice == null) return;

        _subscriptions.Add(_events.Subscribe<StatusReceivedEvent>(async evt =>
        {
            _store.StoreUpdateDeviceStatus(evt.deviceStatus);
        }));
        _subscriptions.Add(_events.Subscribe<ConnectionStatusChangedEvent>(async evt =>
        {
            Status = evt.connectionStatus;
        }));

        _subscriptions.Add(_events.Subscribe<SettingChangedEvent>(evt =>
        {
            if (SelectedDevice.deviceStatus != null)
            {
                _delayToken?.Cancel();
                _delayToken = new CancellationTokenSource();

                var token = _delayToken.Token;
                Task.Delay(50, token).ContinueWith(async t =>
                {
                    if (t.IsCanceled) return;

                    await SendSetting(evt.name, evt.value);
                });
            }
        }));

        _ = ConnectAsync();
    }

    private async Task ConnectAsync()
    {
        if (SelectedDevice == null)
            return;
        await _connection.ConnectAsync(SelectedDevice, _events);
    }

    public async Task SendSetting(string name, float value)
    {
        _connection.Send($"{name}:{value.ToString(System.Globalization.CultureInfo.InvariantCulture)}");
    }

    private async Task ClearAsync()
    {
        await _connection.DisconnectAsync();

        _store.SelectDevice(null);
        Status = null;
    }

    async Task<bool> IHandleBackNavigation.HandleBackAsync()
    {
        await ClearAsync();
        return await Task.FromResult(false);
    }
}
