using Avalonia_Navigation;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using System.Threading.Tasks;
using Avalonia_EventHub;
using System;
using Esp32_Display_Connect.Events;
using System.Collections.ObjectModel;
using System.Collections.Generic;

namespace Esp32_Display_Connect.ViewModels;

public partial class BluetoothViewModel : ViewModelBase, IHandleBackNavigation
{    
    private BluetoothDevice? _selectedBtDevice;
    public BluetoothDevice? SelectedBtDevice
    {
        get => _selectedBtDevice;
        set
        {
            if (value == null || _selectedBtDevice == value) return;

            _selectedBtDevice = value;

            _ = SelectDeviceAsync(_selectedBtDevice);
            _selectedBtDevice = null;
            OnPropertyChanged(nameof(SelectedBtDevice));
        }
    }
    public ObservableCollection<BluetoothDevice>? BtDevicesList { get; } = [];

    public ICommand? RescanCommand { get; }

    private readonly IBluetoothService _bluetooth;

    public BluetoothViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        IBluetoothService bluetooth
    ):base(store, navigator, events)
    {             
        _bluetooth = bluetooth;
  
        _subscriptions.Add(_events.Subscribe<BluetoothDiscoveredEvent>(async evt =>
        {
            BtDevicesList.Add(evt.device);
        }));

        RescanCommand = new AsyncRelayCommand(ScanAsync);

        _ = LoadKnownAsync();
    }

    async Task LoadKnownAsync()
    {
        if (BtDevicesList == null) return;

        IReadOnlyList<BluetoothDevice> known = await _bluetooth.GetKnownDeviceAsync();
        foreach (var device in known)
            BtDevicesList.Add(device);
    }

    async Task ScanAsync()
    {
        _ = await _bluetooth.ScanAsync(TimeSpan.FromSeconds(30), _events);
    }

    private async Task ClearAsync()
    {
        _selectedBtDevice = null;
        OnPropertyChanged(nameof(SelectedBtDevice));
    }

    private async Task SelectDeviceAsync(BluetoothDevice device)
    {
        Console.WriteLine(device.Name);
    }

    async Task<bool> IHandleBackNavigation.HandleBackAsync()
    {
        await ClearAsync();
        return await Task.FromResult(false);
    }
}
