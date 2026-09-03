; Instalador do Vocal Compressor (Windows) - Inno Setup.
; Gerado pela pipeline de CI (.github/workflows/build-windows.yml), que
; compila o plugin via CMake antes de rodar o ISCC contra este script.
; Os caminhos em [Files] são relativos a esta pasta (installer\windows\),
; por isso sobem dois níveis (..\..\) até a raiz do repo / pasta build\.

#define MyAppName "NF - Color Comp"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "NF Plugins"
#define MyAppExeName "NF - Color Comp.exe"

[Setup]
; GUID fixo do app - NÃO trocar entre versões, é o que permite o Windows
; reconhecer upgrade/desinstalação corretamente ao longo do tempo.
AppId={{8F1B7C2E-4A3D-4E9B-9C2F-6D5A1B0E7F44}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
; Agrupado sob "NF Plugins" (igual iZotope/FabFilter fazem com a marca
; deles) - todo plugin novo da linha usa o mesmo padrão de pasta.
DefaultDirName={autopf}\NF Plugins\{#MyAppName}
DefaultGroupName=NF Plugins\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\..\installer_output
OutputBaseFilename=VocalCompressor-Windows-Installer
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}
DisableWelcomePage=no
; Ícone do próprio arquivo .exe do instalador (o que aparece quando a
; pessoa baixa pelo navegador) - logo do produto (NF Color Comp).
SetupIconFile=icon\nf-plugins.ico
; Imagem grande do assistente (lado esquerdo das páginas de boas-vindas/
; conclusão) - mesmo logo do produto, centralizado num fundo roxo/preto
; que combina com o faceplate do plugin.
WizardImageFile=icon\wizard-image.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Standalone (app de verdade, com ícone/atalho)
Source: "..\..\build\VocalCompressor_artefacts\Release\Standalone\NF - Color Comp.exe"; DestDir: "{app}"; Flags: ignoreversion

; VST3 - pasta padrão do Windows pra plugins VST3 de 64 bits, agrupado
; numa subpasta "NF Plugins" (mesmo padrão usado no Program Files/Menu
; Iniciar) pra juntar todos os plugins da marca num lugar só. O VST3 é
; um "bundle" (pasta com subpastas dentro - Contents\x86_64-win\ etc,
; formato exigido pelo próprio padrão VST3, não dá pra achatar), por
; isso recursesubdirs.
Source: "..\..\build\VocalCompressor_artefacts\Release\VST3\NF - Color Comp.vst3\*"; DestDir: "{commoncf64}\VST3\NF Plugins\NF - Color Comp.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; Manual em PDF - fica num local FIXO e compartilhado (ProgramData, não
; dentro de nenhum formato específico), porque o VST3/AAX/Standalone são
; instalados em pastas diferentes e o botão "Manual" da tela About (ver
; Source/UI/AboutOverlay.h) precisa achar o arquivo não importa qual
; formato esteja rodando. É só um PDF estático - não afeta o binário do
; plugin nem carrega em memória até a pessoa clicar.
Source: "..\docs\NF-Color-Comp-Manual-PT.pdf"; DestDir: "{commonappdata}\NF Plugins\Color Comp"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Manual (PT)"; Filename: "{commonappdata}\NF Plugins\Color Comp\NF-Color-Comp-Manual-PT.pdf"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na Área de Trabalho"; GroupDescription: "Atalhos adicionais:"; Flags: unchecked

[UninstallDelete]
; Garante que a pasta inteira do VST3 (bundle) some no uninstall, não só
; os arquivos que o Inno rastreou individualmente.
Type: filesandordirs; Name: "{commoncf64}\VST3\NF Plugins\NF - Color Comp.vst3"
Type: filesandordirs; Name: "{commonappdata}\NF Plugins\Color Comp"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir {#MyAppName} agora"; Flags: nowait postinstall skipifsilent
