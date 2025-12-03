using WebRelay.Server.Example.Blazor.Components;
using WebRelay.Server.Example.Blazor.Services;
using WebRelay.Server.Example.Blazor.Endpoints;

namespace WebRelay.Server.Example.Blazor
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            // Add services to the container.
            builder.Services.AddRazorComponents()
                .AddInteractiveServerComponents();

            // Register relay command service as singleton
            builder.Services.AddSingleton<RelayCommandService>();

            var app = builder.Build();

            // Configure the HTTP request pipeline.
            if (!app.Environment.IsDevelopment())
            {
                app.UseExceptionHandler("/Error");
            }

            app.UseStatusCodePagesWithReExecute("/not-found", createScopeForStatusCodePages: true);
            app.UseAntiforgery();

            app.MapStaticAssets();

            // Map relay API endpoints for ESP32 communication
            // MUST be before MapRazorComponents to avoid Blazor catching the request
            app.MapRelayEndpoints();

            app.MapRazorComponents<App>()
                .AddInteractiveServerRenderMode();

            app.Run();
        }
    }
}
