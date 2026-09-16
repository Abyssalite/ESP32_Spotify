using System.Threading.Tasks;
using Avalonia_EventHub;
using Avalonia_Navigation;
using Microsoft.Extensions.DependencyInjection;

namespace Esp32_Display_Connect.ViewModels;

public partial class MainViewModel : ViewModelBase
{    
    private readonly IViewHost _viewhost;
    public IViewHost ViewHost => _viewhost;
    //private readonly IBluetoothService _bluetooth;

    public MainViewModel(
        Store store,
        IViewHost viewHost,
        INavigatorService navigator,
        IEventHub events
        //IBluetoothService bluetooth
    ):base(store, navigator, events)
    {
        _viewhost = viewHost;
        //_bluetooth = bluetooth;

        _ = InitializeAsync();
        //_ = _bluetooth.ScanAsync();
    }

    public async Task InitializeAsync()
    {        
        _store.DevicesList = await Helpers.LoadAsync() ?? [];

        var vm = App.Services?.GetRequiredService<SelectViewModel>();
        await _navigator.NavigateMain(vm); 
    }
}
