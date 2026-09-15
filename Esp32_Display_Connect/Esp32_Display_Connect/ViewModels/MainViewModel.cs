using System.Threading.Tasks;
using Avalonia_EventHub;
using Avalonia_Navigation;
using Microsoft.Extensions.DependencyInjection;

namespace Esp32_Display_Connect.ViewModels;

public partial class MainViewModel : ViewModelBase
{    
    private readonly IViewHost _viewhost;
    public IViewHost ViewHost => _viewhost;

    public MainViewModel(
        Store store,
        IViewHost viewHost,
        INavigatorService navigator,
        IEventHub events
    ):base(store, navigator, events)
    {
        _viewhost = viewHost;
        _ = InitializeAsync();
    }

    public async Task InitializeAsync()
    {        
        _store.DevicesList = await Helpers.LoadAsync() ?? [];

        var vm = App.Services?.GetRequiredService<SelectViewModel>();
        await _navigator.NavigateMain(vm); 
    }
}
