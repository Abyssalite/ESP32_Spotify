
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Avalonia_Navigation;
using Avalonia_EventHub;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.DependencyInjection;

namespace Esp32_Display_Connect.ViewModels;

public partial class SelectViewModel : ViewModelBase
{    
    public ICommand AddDeviceCommand { get; }

    public ObservableCollection<Device> DevicesList { get; }
    private Device? _selectedDevice;
    public Device? SelectedDevice
    {
        get => _selectedDevice;
        set
        {
            if (value == null || _selectedDevice == value) return;

            _selectedDevice = value;

            _ = OpenDeviceAsync(_selectedDevice);
            _selectedDevice = null;
            OnPropertyChanged(nameof(SelectedDevice));
        }
    }
    
    public SelectViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events
    ):base(store, navigator, events)
    {
        DevicesList = _store.DevicesList;
        AddDeviceCommand = new AsyncRelayCommand(addDeviceAsync);
    }

    async Task addDeviceAsync()
    {
        var vm = App.Services?.GetRequiredService<AddDeviceViewModel>();
        _selectedDevice = null;
        OnPropertyChanged(nameof(SelectedDevice));

        await _navigator.NavigateMain(vm);     
    }

    private async Task OpenDeviceAsync(Device device)
    {
        _store.SelectedDevice = device;

        var vm = App.Services?.GetRequiredService<DeviceViewModel>();
        await _navigator.NavigateMain(vm);     
    }
}
