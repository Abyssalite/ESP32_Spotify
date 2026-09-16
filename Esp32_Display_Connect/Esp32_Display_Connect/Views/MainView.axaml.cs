using Avalonia.Controls;
using Avalonia.Media;

namespace Esp32_Display_Connect.Views;

public partial class MainView : UserControl
{
    public MainView()
    {
        InitializeComponent();

        this.AttachedToVisualTree += (_, __) =>
        {
            var topLevel = TopLevel.GetTopLevel(this);
            var insetsManager = topLevel?.InsetsManager;

            if (insetsManager is not null)
            {
                insetsManager.SystemBarColor = Colors.Black;
                //insetsManager.DisplayEdgeToEdge = true;
            }
        };
    }
}