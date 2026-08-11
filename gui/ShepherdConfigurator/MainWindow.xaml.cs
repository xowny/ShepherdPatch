using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using ShepherdConfigurator.Configuration;
using ShepherdConfigurator.Shell;
using ShepherdConfigurator.Services;
using Windows.Graphics;
using Windows.Storage.Pickers;
using Windows.UI;
using WinRT.Interop;

namespace ShepherdConfigurator;

public sealed partial class MainWindow : Window
{
    private readonly List<CardDefinition> _cards;
    private readonly List<Button> _categoryButtons;
    private readonly List<PairedSettingRow> _pairedSettingRows;
    private readonly List<ValueGridDefinition> _valueGrids;
    private bool _isLoading;
    private bool _isDirty;
    private bool _showAdvancedOptions;
    private bool _isSelectingGameBin;
    private string? _configPath;
    private IniDocument? _loadedDocument;
    private DependencyStatus? _dependencyStatus;
    private IReadOnlyList<RuntimePrerequisite> _missingRuntimePrerequisites = [];
    private string _selectedCategory = "All";
    private AdaptiveLayoutMode _layoutMode;

    public MainWindow()
    {
        InitializeComponent();
        LaunchThroughSteamCheckBox.IsChecked =
            ConfigFileService.ResolveLaunchThroughSteamPreference();

        AppWindow.Title = "ShepherdPatch Configurator";
        AppWindow.Resize(new SizeInt32(1360, 940));
        if (AppWindow.Presenter is OverlappedPresenter presenter)
        {
            presenter.IsResizable = true;
            presenter.IsMaximizable = true;
            presenter.IsMinimizable = true;
        }

        ApplyWindowIcon();
        ApplyTitleBarTheme();
        SizeChanged += MainWindow_SizeChanged;

        _cards =
        [
            new CardDefinition(DisplayCard, "Display", ["display", "resolution", "borderless", "dpi", "ultrawide", "hud", "refresh", "flip"], 7),
            new CardDefinition(InputCard, "Input", ["input", "mouse", "raw", "invert", "directinput", "sensitivity"], 4),
            new CardDefinition(FrameRateCard, "Frame Rate", ["frame", "fps", "vsync", "presentation", "wake", "rate"], 5),
            new CardDefinition(StabilityCard, "Stability", ["stability", "timing", "sleep", "timer", "crash", "logging", "mmcss", "power"], 6),
            new CardDefinition(AdvancedCard, "Advanced", ["advanced", "legacy", "thread", "pointer", "floating", "graphics", "timer"], 5),
            new CardDefinition(MoviesCard, "Movies", ["movie", "movies", "menu", "bink", "stutter"], 2)
        ];

        _categoryButtons =
        [
            AllCategoryButton,
            DisplayCategoryButton,
            InputCategoryButton,
            FrameRateCategoryButton,
            StabilityCategoryButton,
            MoviesCategoryButton,
            AdvancedCategoryButton
        ];

        _pairedSettingRows =
        [
            new PairedSettingRow(DisplayRowOne),
            new PairedSettingRow(DisplayRowTwo),
            new PairedSettingRow(DisplayResolutionUiRow),
            new PairedSettingRow(DisplayRowThree),
            new PairedSettingRow(DisplayRowFour),
            new PairedSettingRow(InputRowOne),
            new PairedSettingRow(InputRowTwo),
            new PairedSettingRow(FrameRateRowOne),
            new PairedSettingRow(StabilityRowOne),
            new PairedSettingRow(StabilityRowTwo),
            new PairedSettingRow(StabilityRowThree),
            new PairedSettingRow(StabilityRowFour),
            new PairedSettingRow(AdvancedRowOne),
            new PairedSettingRow(AdvancedRowTwo),
            new PairedSettingRow(MoviesRowOne)
        ];

        _valueGrids =
        [
            new ValueGridDefinition(DisplayValuesGrid, 2),
            new ValueGridDefinition(InputValuesGrid, 1),
            new ValueGridDefinition(FrameRateValuesGrid, 3),
            new ValueGridDefinition(AdvancedValuesGrid, 2)
        ];

        UpdateCategoryButtonStates();
        LoadConfiguration();
        RefreshDependencyBanner();
        ApplyAdvancedOptionsVisibility();
        UpdateCardVisibility();
        double rasterScale = Content.XamlRoot?.RasterizationScale ?? 1.0;
        double effectiveWidth = AdaptiveLayoutAdvisor.GetEffectiveWidth(
            AppWindow.Size.Width, rasterScale);
        ApplyAdaptiveLayout(AdaptiveLayoutAdvisor.GetMode(effectiveWidth));
    }

    private void LoadConfiguration()
    {
        _configPath = ConfigFileService.ResolveConfigPath();
        if (_configPath is null)
        {
            _loadedDocument = ConfigFileService.LoadDefaults();
            ApplyDocumentToControls(_loadedDocument);
            ConfigPathText.Text = "Choose the game Bin before saving";
            SetDirty(false);
            return;
        }

        if (!File.Exists(_configPath))
        {
            _loadedDocument = ConfigFileService.LoadDefaults();
            ApplyDocumentToControls(_loadedDocument);
            ConfigPathText.Text = $"New configuration will be created: {_configPath}";
            SetDirty(false);
            return;
        }

        if (!ConfigFileService.TryLoad(_configPath, out IniDocument loadedDocument))
        {
            _loadedDocument = ConfigFileService.LoadDefaults();
            ApplyDocumentToControls(_loadedDocument);
            ConfigPathText.Text =
                "ShepherdPatch.ini could not be read. Check access to the file and try again.";
            SetDirty(false);
            return;
        }

        _loadedDocument = loadedDocument;
        ApplyDocumentToControls(_loadedDocument);
        ConfigPathText.Text = _configPath;
        SetDirty(false);
    }

    private void ApplyDocumentToControls(IniDocument document)
    {
        _isLoading = true;
        try
        {
            SetToggle(EnableSafeResolutionChangesToggle, document, "EnableSafeResolutionChanges");
            SetToggle(EnableDpiAwarenessToggle, document, "EnableDpiAwareness");
            SetToggle(EnableUltrawideFovFixToggle, document, "EnableUltrawideFovFix");
            SetToggle(EnableHighResolutionUiFixToggle, document, "EnableHighResolutionUiFix");
            SetToggle(ForceBorderlessToggle, document, "ForceBorderless");
            SetToggle(RetryResetInWindowedModeToggle, document, "RetryResetInWindowedMode");
            SetToggle(EnableHudViewportClampToggle, document, "EnableHudViewportClamp");
            SetToggle(ReduceBorderlessPresentStutterToggle, document, "ReduceBorderlessPresentStutter");
            SetToggle(EnableFlipExSwapEffectToggle, document, "EnableFlipExSwapEffect");
            SetNumber(FallbackWidthBox, document, "FallbackWidth", 0.0);
            SetNumber(FallbackHeightBox, document, "FallbackHeight", 0.0);
            SetNumber(HudViewportAspectRatioBox, document, "HudViewportAspectRatio", 16.0 / 9.0);
            SetNumber(FallbackRefreshRateBox, document, "FallbackRefreshRate", 60.0);

            SetToggle(EnableRawMouseInputToggle, document, "EnableRawMouseInput");
            SetToggle(InvertRawMouseYToggle, document, "InvertRawMouseY");
            SetToggle(HardenDirectInputMouseDeviceToggle, document, "HardenDirectInputMouseDevice");
            SetNumber(RawMouseSensitivityBox, document, "RawMouseSensitivity", 1.0);

            SetToggle(EnableFrameRateUnlockToggle, document, "EnableFrameRateUnlock");
            SetToggle(DisableVsyncToggle, document, "DisableVsync");
            SetNumber(TargetFrameRateBox, document, "TargetFrameRate", 60.0);
            SetNumber(MaximumPresentationIntervalBox, document, "MaximumPresentationInterval", 1.0);
            SetNumber(PresentWakeLeadMillisecondsBox, document, "PresentWakeLeadMilliseconds", 0.75);

            SetToggle(EnableHighPrecisionTimingToggle, document, "EnableHighPrecisionTiming");
            SetToggle(RequestHighResolutionTimerToggle, document, "RequestHighResolutionTimer");
            SetToggle(EnablePreciseSleepShimToggle, document, "EnablePreciseSleepShim");
            SetToggle(EnableCrashDumpsToggle, document, "EnableCrashDumps");
            SetToggle(SynchronizeEngineVarsToggle, document, "SynchronizeEngineVars");
            SetToggle(EnableGamesMmcssProfileToggle, document, "EnableGamesMmcssProfile");
            SetToggle(DisablePowerThrottlingToggle, document, "DisablePowerThrottling");

            SetToggle(DisableLegacyPeriodicIconicTimerToggle, document, "DisableLegacyPeriodicIconicTimer");
            SetToggle(ReduceLegacyWaitableTimerPollingToggle, document, "ReduceLegacyWaitableTimerPolling");
            SetToggle(HardenLegacyGraphicsRecoveryToggle, document, "HardenLegacyGraphicsRecovery");
            SetToggle(HardenLegacyThreadWrapperToggle, document, "HardenLegacyThreadWrapper");
            SetNumber(LegacyGraphicsRetryDelayMillisecondsBox, document, "LegacyGraphicsRetryDelayMilliseconds", 100.0);
            SetNumber(LegacyThreadTerminateGraceMillisecondsBox, document, "LegacyThreadTerminateGraceMilliseconds", 250.0);

            SetToggle(ReduceMenuMovieStutterToggle, document, "ReduceMenuMovieStutter");
        }
        finally
        {
            _isLoading = false;
        }
    }

    private void SetToggle(ToggleButton toggle, IniDocument document, string key)
    {
        toggle.IsChecked = ParseBool(document.GetValue(key));
    }

    private static bool ParseBool(string? value)
    {
        return string.Equals(value, "true", StringComparison.OrdinalIgnoreCase) || value == "1";
    }

    private static void SetNumber(NumberBox numberBox, IniDocument document, string key, double fallback)
    {
        if (double.TryParse(document.GetValue(key), NumberStyles.Float,
                CultureInfo.InvariantCulture, out double value))
        {
            numberBox.Value = value;
            return;
        }

        numberBox.Value = fallback;
    }

    private IniDocument BuildDocumentFromControls()
    {
        IniDocument document = _loadedDocument is not null
            ? IniDocument.Parse(_loadedDocument.Serialize())
            : IniDocument.Parse(string.Empty);

        SetValue(document, "EnableSafeResolutionChanges", EnableSafeResolutionChangesToggle.IsChecked == true);
        SetValue(document, "EnableDpiAwareness", EnableDpiAwarenessToggle.IsChecked == true);
        SetValue(document, "EnableUltrawideFovFix", EnableUltrawideFovFixToggle.IsChecked == true);
        SetValue(document, "EnableHighResolutionUiFix", EnableHighResolutionUiFixToggle.IsChecked == true);
        SetValue(document, "ForceBorderless", ForceBorderlessToggle.IsChecked == true);
        SetValue(document, "RetryResetInWindowedMode", RetryResetInWindowedModeToggle.IsChecked == true);
        SetValue(document, "EnableHudViewportClamp", EnableHudViewportClampToggle.IsChecked == true);
        SetValue(document, "ReduceBorderlessPresentStutter", ReduceBorderlessPresentStutterToggle.IsChecked == true);
        SetValue(document, "EnableFlipExSwapEffect", EnableFlipExSwapEffectToggle.IsChecked == true);
        SetFiniteValue(document, "FallbackWidth", FallbackWidthBox.Value, 0.0, 0);
        SetFiniteValue(document, "FallbackHeight", FallbackHeightBox.Value, 0.0, 0);
        SetFiniteValue(document, "HudViewportAspectRatio", HudViewportAspectRatioBox.Value, 16.0 / 9.0);
        SetFiniteValue(document, "FallbackRefreshRate", FallbackRefreshRateBox.Value, 60.0, 0);

        SetValue(document, "EnableRawMouseInput", EnableRawMouseInputToggle.IsChecked == true);
        SetValue(document, "InvertRawMouseY", InvertRawMouseYToggle.IsChecked == true);
        SetValue(document, "HardenDirectInputMouseDevice", HardenDirectInputMouseDeviceToggle.IsChecked == true);
        SetFiniteValue(document, "RawMouseSensitivity", RawMouseSensitivityBox.Value, 1.0);

        SetValue(document, "EnableFrameRateUnlock", EnableFrameRateUnlockToggle.IsChecked == true);
        SetValue(document, "DisableVsync", DisableVsyncToggle.IsChecked == true);
        SetFiniteValue(document, "TargetFrameRate", TargetFrameRateBox.Value, 60.0, 0);
        SetFiniteValue(document, "MaximumPresentationInterval", MaximumPresentationIntervalBox.Value, 1.0, 0);
        SetFiniteValue(document, "PresentWakeLeadMilliseconds", PresentWakeLeadMillisecondsBox.Value, 0.75);

        SetValue(document, "EnableHighPrecisionTiming", EnableHighPrecisionTimingToggle.IsChecked == true);
        SetValue(document, "RequestHighResolutionTimer", RequestHighResolutionTimerToggle.IsChecked == true);
        SetValue(document, "EnablePreciseSleepShim", EnablePreciseSleepShimToggle.IsChecked == true);
        SetValue(document, "EnableCrashDumps", EnableCrashDumpsToggle.IsChecked == true);
        SetValue(document, "SynchronizeEngineVars", SynchronizeEngineVarsToggle.IsChecked == true);
        SetValue(document, "EnableGamesMmcssProfile", EnableGamesMmcssProfileToggle.IsChecked == true);
        SetValue(document, "DisablePowerThrottling", DisablePowerThrottlingToggle.IsChecked == true);

        SetValue(document, "DisableLegacyPeriodicIconicTimer", DisableLegacyPeriodicIconicTimerToggle.IsChecked == true);
        SetValue(document, "ReduceLegacyWaitableTimerPolling", ReduceLegacyWaitableTimerPollingToggle.IsChecked == true);
        SetValue(document, "HardenLegacyGraphicsRecovery", HardenLegacyGraphicsRecoveryToggle.IsChecked == true);
        SetValue(document, "HardenLegacyThreadWrapper", HardenLegacyThreadWrapperToggle.IsChecked == true);
        SetFiniteValue(document, "LegacyGraphicsRetryDelayMilliseconds", LegacyGraphicsRetryDelayMillisecondsBox.Value, 100.0, 0);
        SetFiniteValue(document, "LegacyThreadTerminateGraceMilliseconds", LegacyThreadTerminateGraceMillisecondsBox.Value, 250.0, 0);

        SetValue(document, "ReduceMenuMovieStutter", ReduceMenuMovieStutterToggle.IsChecked == true);

        return document;
    }

    private static void SetValue(IniDocument document, string key, bool value)
    {
        document.SetValue(key, value ? "true" : "false");
    }

    private static void SetFiniteValue(
        IniDocument document, string key, double value, double fallback, int decimalPlaces = 3)
    {
        value = double.IsFinite(value) ? value : fallback;
        string format = decimalPlaces <= 0 ? "0" : $"0.{new string('#', decimalPlaces)}";
        document.SetValue(key, value.ToString(format, CultureInfo.InvariantCulture));
    }

    private void SetDirty(bool value)
    {
        _isDirty = value;
        DirtyIndicator.Background = new SolidColorBrush(value ? Windows.UI.Color.FromArgb(0x55, 0x8A, 0x2E, 0x34) : Windows.UI.Color.FromArgb(0x33, 0x10, 0x10, 0x10));
        DirtyIndicatorText.Text = value ? "Unsaved changes" : "No pending changes";
        UpdateContextSummary();
    }

    private void UpdateCardVisibility()
    {
        if (SearchBox is null)
        {
            return;
        }

        string query = SearchBox.Text ?? string.Empty;
        IReadOnlyList<CardDefinition> visibleCards = CardVisibilityFilter.FilterVisible(
            _cards,
            _selectedCategory,
            query,
            static card => card.Category,
            static card => card.Tokens);

        if (!_showAdvancedOptions)
        {
            visibleCards = visibleCards
                .Where(static card => card.Category is "Display" or "Input" or "Frame Rate")
                .ToList();
        }

        foreach (CardDefinition card in _cards)
        {
            bool visible = visibleCards.Contains(card);
            card.Element.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        }

        ApplyCardGridLayout(_layoutMode);
        UpdateContextSummary();
    }

    private void AnySettingChanged(object sender, RoutedEventArgs e)
    {
        if (_isLoading)
        {
            return;
        }

        SetDirty(true);
    }

    private void AnyNumberChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
    {
        if (_isLoading)
        {
            return;
        }

        SetDirty(true);
    }

    private void CategoryButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not Button button)
        {
            return;
        }

        _selectedCategory = button.Tag?.ToString() ?? "All";
        UpdateCategoryButtonStates();
        UpdateCardVisibility();
    }

    private void SearchBox_TextChanged(object sender, TextChangedEventArgs args)
    {
        UpdateCardVisibility();
    }

    private void AdvancedOptionsToggle_Click(object sender, RoutedEventArgs e)
    {
        _showAdvancedOptions = AdvancedOptionsToggle.IsChecked == true;
        if (!_showAdvancedOptions &&
            _selectedCategory is "Stability" or "Movies" or "Advanced")
        {
            _selectedCategory = "All";
        }

        ApplyAdvancedOptionsVisibility();
        UpdateCategoryButtonStates();
        UpdateCardVisibility();
    }

    private void ApplyAdvancedOptionsVisibility()
    {
        Visibility advancedVisibility = _showAdvancedOptions
            ? Visibility.Visible
            : Visibility.Collapsed;

        StabilityCategoryButton.Visibility = advancedVisibility;
        MoviesCategoryButton.Visibility = advancedVisibility;
        AdvancedCategoryButton.Visibility = advancedVisibility;

        DisplayRowOne.Visibility = advancedVisibility;
        DisplayRowThree.Visibility = advancedVisibility;
        DisplayRowFour.Visibility = advancedVisibility;
        DisplayHudAspectPanel.Visibility = advancedVisibility;
        DisplayFallbackRefreshPanel.Visibility = advancedVisibility;
        InputRowTwo.Visibility = advancedVisibility;
        MaximumPresentationIntervalPanel.Visibility = advancedVisibility;
        PresentWakeLeadPanel.Visibility = advancedVisibility;
    }

    private async void SelectGameBinButton_Click(object sender, RoutedEventArgs e)
    {
        if (_isSelectingGameBin)
        {
            return;
        }

        _isSelectingGameBin = true;
        SelectGameBinButton.IsEnabled = false;
        try
        {
            FolderPicker picker = new();
            picker.FileTypeFilter.Add("*");
            InitializeWithWindow.Initialize(picker, WindowNative.GetWindowHandle(this));

            Windows.Storage.StorageFolder? folder = await picker.PickSingleFolderAsync();
            if (folder is null || !await ConfirmConfigurationSwitchAsync())
            {
                return;
            }

            if (!ConfigFileService.SetSelectedGameBinDirectory(folder.Path))
            {
                ConfigPathText.Text = "Select the Bin folder that contains SilentHill.exe";
                return;
            }

            LoadConfiguration();
            RefreshDependencyBanner();
        }
        catch (Exception)
        {
            ConfigPathText.Text = "The game Bin picker could not be opened. Try again.";
        }
        finally
        {
            _isSelectingGameBin = false;
            SelectGameBinButton.IsEnabled = true;
        }
    }

    private async System.Threading.Tasks.Task<bool> ConfirmConfigurationSwitchAsync()
    {
        if (!_isDirty)
        {
            return true;
        }

        ContentDialog dialog = new()
        {
            XamlRoot = ShellBorder.XamlRoot,
            Title = "Unsaved changes",
            Content = "Save changes to the current installation before choosing another Bin?",
            PrimaryButtonText = "Save",
            SecondaryButtonText = "Discard",
            CloseButtonText = "Cancel",
            DefaultButton = ContentDialogButton.Primary
        };

        ContentDialogResult result = await dialog.ShowAsync();
        return result switch
        {
            ContentDialogResult.Primary => SaveConfiguration(),
            ContentDialogResult.Secondary => true,
            _ => false
        };
    }

    private void SaveButton_Click(object sender, RoutedEventArgs e)
    {
        SaveConfiguration();
    }

    private bool SaveConfiguration()
    {
        if (_configPath is null)
        {
            ConfigPathText.Text = "Choose the game Bin before saving";
            return false;
        }

        IniDocument document = BuildDocumentFromControls();
        try
        {
            ConfigFileService.Save(_configPath, document);
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException)
        {
            ConfigPathText.Text = "ShepherdPatch.ini could not be saved. Check that the file is writable and try again.";
            return false;
        }

        _loadedDocument = document;
        SetDirty(false);
        RefreshDependencyBanner();
        return true;
    }

    private void RunButton_Click(object sender, RoutedEventArgs e)
    {
        if (_isDirty && !SaveConfiguration())
        {
            return;
        }

        if (_dependencyStatus is null)
        {
            RefreshDependencyBanner();
        }

        if (_dependencyStatus is null)
        {
            ConfigPathText.Text = "The game could not be launched from the selected Bin";
            return;
        }

        bool launchThroughSteam = LaunchThroughSteamCheckBox.IsChecked == true;
        GameLaunchPlan? launchPlan = GameLaunchPlanner.Create(
            _dependencyStatus.InstallDirectory,
            launchThroughSteam);
        if (launchPlan is null)
        {
            ConfigPathText.Text = launchThroughSteam
                ? "Steam launch is unavailable for this Bin. Clear 'Launch through Steam' to run SilentHill.exe directly."
                : "SilentHill.exe was not found in the selected Bin";
            return;
        }

        if (!ModDependencyService.LaunchGame(launchPlan))
        {
            ConfigPathText.Text = launchThroughSteam
                ? "Steam could not launch the game"
                : "SilentHill.exe could not be launched from the selected Bin";
        }
    }

    private void LaunchThroughSteamCheckBox_Click(object sender, RoutedEventArgs e)
    {
        bool requestedValue = LaunchThroughSteamCheckBox.IsChecked == true;
        if (ConfigFileService.SetLaunchThroughSteamPreference(requestedValue))
        {
            return;
        }

        LaunchThroughSteamCheckBox.IsChecked =
            ConfigFileService.ResolveLaunchThroughSteamPreference();
        ConfigPathText.Text = "The Steam launch preference could not be saved";
    }

    private void DiscardButton_Click(object sender, RoutedEventArgs e)
    {
        if (_loadedDocument is null)
        {
            return;
        }

        ApplyDocumentToControls(_loadedDocument);
        SetDirty(false);
    }

    private void RefreshDependenciesButton_Click(object sender, RoutedEventArgs e)
    {
        RefreshDependencyBanner();
    }

    private void OpenDependencyInstallFolderButton_Click(object sender, RoutedEventArgs e)
    {
        if (_dependencyStatus is null)
        {
            RefreshDependencyBanner();
        }

        if (_dependencyStatus is not null)
        {
            ModDependencyService.OpenDirectory(_dependencyStatus.InstallDirectory);
        }
    }

    private void OpenBundledDependenciesButton_Click(object sender, RoutedEventArgs e)
    {
        if (_dependencyStatus is null)
        {
            RefreshDependencyBanner();
        }

        if (!string.IsNullOrWhiteSpace(_dependencyStatus?.BundledSourceDirectory))
        {
            ModDependencyService.OpenDirectory(_dependencyStatus.BundledSourceDirectory!);
        }
    }

    private void DownloadRuntimeDependenciesButton_Click(object sender, RoutedEventArgs e)
    {
        if (_missingRuntimePrerequisites.Count == 0)
        {
            RefreshDependencyBanner();
        }

        foreach (RuntimePrerequisite prerequisite in _missingRuntimePrerequisites)
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = prerequisite.DownloadUrl,
                UseShellExecute = true
            });
        }
    }

    private void UpdateCategoryButtonStates()
    {
        foreach (Button button in _categoryButtons)
        {
            bool selected = string.Equals(button.Tag?.ToString(), _selectedCategory, StringComparison.OrdinalIgnoreCase);
            button.Background = (Brush)Application.Current.Resources[selected ? "HomecomingAccentBrush" : "HomecomingSurfaceBrush"];
            button.BorderBrush = (Brush)Application.Current.Resources[selected ? "HomecomingAccentStrongBrush" : "HomecomingSurfaceBorderBrush"];
            button.Foreground = (Brush)Application.Current.Resources[selected ? "HomecomingTextBrush" : "HomecomingMutedTextBrush"];
        }
    }

    private void RefreshDependencyBanner()
    {
        _dependencyStatus = ModDependencyService.GetStatus();
        _missingRuntimePrerequisites = RuntimePrerequisiteAnalyzer.GetMissingPrerequisites();

        bool hasMissingModFiles = _dependencyStatus.HasMissingDependencies;
        bool hasMissingRuntimePrerequisites = _missingRuntimePrerequisites.Count > 0;

        if (!hasMissingModFiles && !hasMissingRuntimePrerequisites)
        {
            DependencyBanner.Visibility = Visibility.Collapsed;
            return;
        }

        List<string> messageParts = [];

        if (hasMissingModFiles)
        {
            string missingFiles = string.Join(", ", _dependencyStatus.MissingFiles);
            messageParts.Add($"Missing mod files: {missingFiles}.");
        }

        if (hasMissingRuntimePrerequisites)
        {
            string missingRuntimeNames = string.Join(", ", _missingRuntimePrerequisites.Select(static item => item.DisplayName));
            messageParts.Add($"Missing runtime requirements: {missingRuntimeNames}.");
        }

        DependencyBannerTitleText.Text = hasMissingRuntimePrerequisites && !hasMissingModFiles
            ? "Required runtime setup missing"
            : "Required setup items missing";
        DependencyBannerText.Text = string.Join(" ", messageParts) + " Install them before launching the game.";
        DependencyPathText.Text = hasMissingModFiles
            ? string.IsNullOrWhiteSpace(_dependencyStatus.InstallDirectory)
                ? "No game Bin selected"
                : $"Target install: {_dependencyStatus.InstallDirectory}"
            : "This build needs extra Windows runtime components before it can run normally.";
        OpenDependencyInstallFolderButton.Visibility = hasMissingModFiles &&
            !string.IsNullOrWhiteSpace(_dependencyStatus.InstallDirectory) &&
            Directory.Exists(_dependencyStatus.InstallDirectory)
                ? Visibility.Visible
                : Visibility.Collapsed;
        OpenBundledDependenciesButton.Visibility = hasMissingModFiles && !string.IsNullOrWhiteSpace(_dependencyStatus.BundledSourceDirectory)
            ? Visibility.Visible
            : Visibility.Collapsed;
        DownloadRuntimeDependenciesButton.Visibility = hasMissingRuntimePrerequisites ? Visibility.Visible : Visibility.Collapsed;
        DependencyBanner.Visibility = Visibility.Visible;
    }

    private void ApplyWindowIcon()
    {
        string iconPath = Path.Combine(AppContext.BaseDirectory, "Assets", "AppIcon.ico");
        if (File.Exists(iconPath))
        {
            AppWindow.SetIcon(iconPath);
        }
    }

    private void ApplyTitleBarTheme()
    {
        if (!AppWindowTitleBar.IsCustomizationSupported())
        {
            return;
        }

        AppWindowTitleBar titleBar = AppWindow.TitleBar;
        titleBar.BackgroundColor = Color.FromArgb(255, 15, 13, 12);
        titleBar.ForegroundColor = Color.FromArgb(255, 242, 233, 219);
        titleBar.InactiveBackgroundColor = Color.FromArgb(255, 19, 17, 15);
        titleBar.InactiveForegroundColor = Color.FromArgb(255, 201, 181, 154);
        titleBar.ButtonBackgroundColor = Color.FromArgb(255, 15, 13, 12);
        titleBar.ButtonForegroundColor = Color.FromArgb(255, 242, 233, 219);
        titleBar.ButtonHoverBackgroundColor = Color.FromArgb(255, 30, 26, 23);
        titleBar.ButtonHoverForegroundColor = Color.FromArgb(255, 242, 233, 219);
        titleBar.ButtonPressedBackgroundColor = Color.FromArgb(255, 185, 90, 37);
        titleBar.ButtonPressedForegroundColor = Color.FromArgb(255, 242, 233, 219);
        titleBar.ButtonInactiveBackgroundColor = Color.FromArgb(255, 19, 17, 15);
        titleBar.ButtonInactiveForegroundColor = Color.FromArgb(255, 201, 181, 154);
    }

    private void MainWindow_SizeChanged(object sender, WindowSizeChangedEventArgs args)
    {
        ApplyAdaptiveLayout(AdaptiveLayoutAdvisor.GetMode(args.Size.Width));
    }

    private void ApplyAdaptiveLayout(AdaptiveLayoutMode mode)
    {
        _layoutMode = mode;

        bool compact = mode == AdaptiveLayoutMode.Compact;
        ShellBorder.Margin = compact
            ? new Thickness(6)
            : mode == AdaptiveLayoutMode.Wide ? new Thickness(14) : new Thickness(10);
        SearchBox.Width = compact ? 180 : mode == AdaptiveLayoutMode.Wide ? 320 : 240;
        ConfigPathText.Width = compact ? 180 : 300;

        foreach (PairedSettingRow row in _pairedSettingRows)
        {
            ApplyPairedRowLayout(row.Grid, compact);
        }

        foreach (ValueGridDefinition valueGrid in _valueGrids)
        {
            ApplyValueGridLayout(valueGrid, compact);
        }

        ApplyFooterLayout(compact);
        ApplyCardGridLayout(mode);
    }

    private void ApplyFooterLayout(bool compact)
    {
        FooterGrid.ColumnDefinitions.Clear();
        FooterGrid.RowDefinitions.Clear();

        FooterGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        FooterGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

        Grid.SetColumn(FooterStatusPanel, 0);
        Grid.SetRow(FooterStatusPanel, 0);

        if (compact)
        {
            FooterGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            Grid.SetColumn(FooterActionsPanel, 0);
            Grid.SetRow(FooterActionsPanel, 1);
            FooterActionsPanel.Orientation = Orientation.Vertical;
            FooterActionsPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            FooterActionsPanel.Margin = new Thickness(0, 8, 0, 0);
            FooterButtonsPanel.HorizontalAlignment = HorizontalAlignment.Right;
            return;
        }

        FooterGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        Grid.SetColumn(FooterActionsPanel, 1);
        Grid.SetRow(FooterActionsPanel, 0);
        FooterActionsPanel.Orientation = Orientation.Horizontal;
        FooterActionsPanel.HorizontalAlignment = HorizontalAlignment.Right;
        FooterActionsPanel.Margin = default;
        FooterButtonsPanel.HorizontalAlignment = HorizontalAlignment.Right;
    }

    private static void ApplyPairedRowLayout(Grid grid, bool compact)
    {
        List<UIElement> children = grid.Children.ToList();
        grid.ColumnDefinitions.Clear();
        grid.RowDefinitions.Clear();

        if (compact)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            for (int i = 0; i < children.Count; i++)
            {
                FrameworkElement element = (FrameworkElement)children[i];
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                Grid.SetColumn(element, 0);
                Grid.SetRow(element, i);

                if (element is StackPanel panel)
                {
                    ConfigureStackPanelAlignment(panel, alignRight: false);
                    panel.Margin = i == 0 ? default : new Thickness(0, 10, 0, 0);
                }
            }

            return;
        }

        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(220) });
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

        for (int i = 0; i < children.Count; i++)
        {
            FrameworkElement element = (FrameworkElement)children[i];
            Grid.SetColumn(element, Math.Min(i, 1));
            Grid.SetRow(element, 0);

            if (element is StackPanel panel)
            {
                ConfigureStackPanelAlignment(panel, alignRight: i == 1);
                panel.Margin = default;
            }
        }
    }

    private static void ApplyValueGridLayout(ValueGridDefinition definition, bool compact)
    {
        List<UIElement> children = definition.Grid.Children.ToList();
        definition.Grid.ColumnDefinitions.Clear();
        definition.Grid.RowDefinitions.Clear();

        if (compact)
        {
            definition.Grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            for (int i = 0; i < children.Count; i++)
            {
                FrameworkElement element = (FrameworkElement)children[i];
                definition.Grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                Grid.SetColumn(element, 0);
                Grid.SetRow(element, i);

                element.Margin = i == children.Count - 1 ? default : new Thickness(0, 0, 0, 12);
            }

            return;
        }

        int columnCount = Math.Max(1, definition.DefaultColumnCount);
        for (int i = 0; i < columnCount; i++)
        {
            definition.Grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        }

        int rowCount = AdaptiveLayoutAdvisor.GetGridRowCount(children.Count, columnCount);
        for (int i = 0; i < rowCount; i++)
        {
            definition.Grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        }

        for (int i = 0; i < children.Count; i++)
        {
            FrameworkElement element = (FrameworkElement)children[i];
            AdaptiveLayoutAdvisor.GridPosition position = AdaptiveLayoutAdvisor.GetGridPosition(i, columnCount);
            Grid.SetColumn(element, position.Column);
            Grid.SetRow(element, position.Row);

            element.Margin = position.Row == rowCount - 1 ? default : new Thickness(0, 0, 0, 12);
        }
    }

    private static void ConfigureStackPanelAlignment(StackPanel panel, bool alignRight)
    {
        panel.HorizontalAlignment = alignRight ? HorizontalAlignment.Right : HorizontalAlignment.Stretch;

        foreach (UIElement child in panel.Children)
        {
            if (child is CheckBox checkBox)
            {
                checkBox.HorizontalAlignment = alignRight ? HorizontalAlignment.Right : HorizontalAlignment.Left;
            }

            if (child is TextBlock textBlock)
            {
                textBlock.TextAlignment = alignRight ? TextAlignment.Right : TextAlignment.Left;
            }
        }
    }

    private void ApplyCardGridLayout(AdaptiveLayoutMode mode)
    {
        List<CardDefinition> visibleCards = _cards.Where(card => card.Element.Visibility == Visibility.Visible).ToList();

        CardsGrid.ColumnDefinitions.Clear();
        CardsGrid.RowDefinitions.Clear();

        int columnCount = AdaptiveLayoutAdvisor.GetColumnCount(mode);
        for (int i = 0; i < columnCount; i++)
        {
            CardsGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        }

        IReadOnlyList<IReadOnlyList<CardDefinition>> columns = AdaptiveLayoutAdvisor.DistributeCards(
            visibleCards,
            static card => card.EstimatedWeight,
            mode);

        int rowCount = Math.Max(1, columns.Max(static column => column.Count));
        for (int i = 0; i < rowCount; i++)
        {
            CardsGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        }

        foreach (CardDefinition card in _cards)
        {
            if (card.Element is FrameworkElement element)
            {
                element.Margin = default;
            }
        }

        for (int columnIndex = 0; columnIndex < columns.Count; columnIndex++)
        {
            IReadOnlyList<CardDefinition> column = columns[columnIndex];
            for (int rowIndex = 0; rowIndex < column.Count; rowIndex++)
            {
                FrameworkElement element = column[rowIndex].Element;
                Grid.SetRow(element, rowIndex);
                Grid.SetColumn(element, columnIndex);
            }
        }
    }

    private void UpdateContextSummary()
    {
        if (ContextSummaryText is null)
        {
            return;
        }

        List<CardDefinition> visibleCards = _cards.Where(card => card.Element.Visibility == Visibility.Visible).ToList();
        int experimentalCount = ExperimentalSettingCatalog.CountForCategories(visibleCards.Select(card => card.Category));
        string query = SearchBox?.Text?.Trim() ?? string.Empty;

        ContextSummaryText.Text = ConfiguratorStatusSummary.Build(
            visibleSectionCount: visibleCards.Count,
            selectedCategory: _selectedCategory,
            query: query,
            isDirty: _isDirty,
            visibleExperimentalCount: experimentalCount);

        // The context summary already includes the experimental-option count.
        // Keeping a second status pill in the command row wastes scarce width.
        ExperimentalNotice.Visibility = Visibility.Collapsed;
        ExperimentalNoticeText.Text = experimentalCount == 1
            ? "1 experimental option visible"
            : $"{experimentalCount} experimental options visible";
    }

    private sealed record CardDefinition(FrameworkElement Element, string Category, string[] Tokens, int EstimatedWeight);
    private sealed record PairedSettingRow(Grid Grid);
    private sealed record ValueGridDefinition(Grid Grid, int DefaultColumnCount);
}
