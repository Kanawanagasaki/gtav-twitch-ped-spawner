using GtaV.TwitchPedSpawner.ApiClient;
using GtaV.TwitchPedSpawner.Components;
using GtaV.TwitchPedSpawner.Services;

var builder = WebApplication.CreateBuilder(args);

builder.WebHost.UseUrls("http://localhost:5376");

builder.Services.AddDbContext<MySqlContext>();
builder.Services.AddRazorComponents().AddInteractiveWebAssemblyComponents();
builder.Services.AddControllers();

builder.Services.AddScoped<JwtService>();
builder.Services.AddHttpClient<TwitchApiClient>("TwitchApi", (http) =>
{
    http.BaseAddress = new Uri("https://api.twitch.tv");
    http.DefaultRequestHeaders.Add("Client-Id", builder.Configuration["Twitch:ClientId"]);
});

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    app.UseWebAssemblyDebugging();
}
else
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
}

app.UseStaticFiles();
app.UseAntiforgery();

app.MapControllers();

app.MapRazorComponents<App>()
    .AddInteractiveWebAssemblyRenderMode()
    .AddAdditionalAssemblies(typeof(GtaV.TwitchPedSpawner.Client._Imports).Assembly);

app.Run();
