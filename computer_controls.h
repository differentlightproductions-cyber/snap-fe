#ifndef SNAPFE_COMPUTER_CONTROLS_H
#define SNAPFE_COMPUTER_CONTROLS_H
/* Read-only help. Knulli/configgen remains the input authority. Sources and
   firmware limitations are recorded in COMPUTER-CONTROLS.md. */
static int syshelp_scroll=0;
static char syshelp_lines[72][128];
static int syshelp_count=0;
static void syshelp_add(const char *text) {
    char wrapped[MAX_LINES][128];int n=wrap_text(font_label,text,WIN_W-80,wrapped);
    for(int i=0;i<n&&syshelp_count<72;i++)snprintf(syshelp_lines[syshelp_count++],128,"%s",wrapped[i]);
    if(syshelp_count<72)syshelp_lines[syshelp_count++][0]=0;
}
static void syshelp_open(int p) {
    syshelp_count=syshelp_scroll=0;const char *s=platform_dirs[p];
    syshelp_add("Knulli defaults are used. Custom core or game remaps may differ from these tips.");
    if(!strncmp(s,"amiga",5)){
        syshelp_add("Amiga / PUAE: D-pad is joystick. Select opens the on-screen keyboard; Start presses Return.");
        syshelp_add("Mouse game? Open the keyboard with Select and choose J/M to switch the D-pad to mouse control. L2/R2 click the mouse buttons on Knulli.");
        syshelp_add("Use the keyboard to answer trainer/startup questions. Core Options lets you change the emulated machine and joystick/mouse mode per game.");
        syshelp_add("Kickstart BIOS belongs in SHARE/bios/amiga. AROS fallback can start some games without a real Kickstart, but compatibility is limited.");
    }else if(!strcmp(s,"atari800")){
        syshelp_add("Atari800 core: Y opens the keyboard. Start starts the Atari game; Select is the Atari Select key. A fires; B presses Return.");
        syshelp_add("L1 is Atari Option; R1 opens the Atari computer menu. L2 is Space; R2 is Escape.");
        syshelp_add("If input was changed in RetroArch, Quick Menu > Controls > Port 1 must use ATARI Joystick, not ordinary RetroPad.");
        syshelp_add("Computer BIOS files include ATARIXL.ROM and ATARIBAS.ROM in SHARE/bios. The right OS depends on the emulated Atari model.");
    }else if(!strcmp(s,"c64")){
        syshelp_add("C64 / VICE: D-pad is joystick. Tap Select for the on-screen keyboard; use B to press its keys and Start for Return.");
        syshelp_add("Wrong joystick port can look like dead controls. Choose JOYP/JOY on the virtual keyboard to switch ports, or set RetroPad Port in Core Options.");
        syshelp_add("Older games often use port 1, later games port 2. Save a Game Options File for that title rather than remapping every system.");
        syshelp_add("VICE includes standard C64 firmware. Optional JiffyDOS is not required for normal games.");
    }else if(!strcmp(s,"zxspectrum")){
        syshelp_add("Spectrum / Fuse: Select opens the keyboard. A/X/Y fire; B is up/jump. L1 presses Return; R1 presses Space.");
        syshelp_add("Use the game's own menu to choose the same joystick type as RetroArch Port 1, for example Kempston or Sinclair.");
        syshelp_add("Do not enable a physical keyboard device in another port unless using one; it can conflict with the built-in keyboard shortcuts.");
    }else if(!strcmp(s,"amstradcpc")){
        syshelp_add("Amstrad CPC: Knulli selects the cap32 core and its Amstrad joystick device. Some disk games stop at a BASIC or keyboard-controlled menu.");
        syshelp_add("Open RetroArch's Quick Menu > Controls to see the active controller actions. Use the core's virtual keyboard for disk commands or game menu keys.");
    }else if(!strcmp(s,"atarist")){
        syshelp_add("Atari ST games can require mouse input, joystick input or a keyboard command. Hatari's input options select the needed mode; one mapping cannot suit all three.");
        syshelp_add("Hatari requires a compatible TOS image in SHARE/bios/tos.img. A missing BIOS is a firmware problem, not a controller mapping problem.");
    }else if(!strncmp(s,"msx",3)){
        syshelp_add("MSX games may need a keyboard start command or a different joystick port. Select the matching controller/port in the active core's options.");
        syshelp_add("blueMSX needs its Machines and Databases folders under SHARE/bios. Keep their internal directory structure intact.");
    }else{
        syshelp_add("SNAP passes the built-in controller's identity to the same firmware launcher used by EmulationStation. The selected emulator configures this system's inputs.");
        syshelp_add("For a RetroArch game, open Quick Menu > Controls to see its button actions. Computer, keypad and mouse games may need different core device types or a virtual keyboard.");
    }
    syshelp_add("RetroArch menu default: Menu + Select. Save button remaps with Manage Remap Files > Save Game Remap File; save core options with Manage Core Options > Save Game Options File.");
    syshelp_add("Keep built-in controls configured by Knulli. Do not remap the whole handheld in EmulationStation to repair one computer game.");
    syshelp_add("Missing BIOS does not always prevent launching. Add only firmware you legally obtained. COMPUTER-CONTROLS.md and tools/audit_system_controls.py provide the wider diagnostic route.");
}
static void syshelp_draw(SDL_Renderer *ren,Theme *th,int p) {
    draw_theme_background(ren,th,theme_idx);
    boot_text_block(ren,font_small,"CONTROLS & BIOS",th->accent2,(SDL_Rect){40,40,WIN_W-80,36});
    boot_text_block(ren,font_label,platform_names[p],g_ui_text,(SDL_Rect){40,82,WIN_W-80,28});
    int pitch=TTF_FontHeight(font_label)+4,visible=(WIN_H-160)/pitch;
    int max=syshelp_count-visible;if(max<0)max=0;
    if(syshelp_scroll>max)syshelp_scroll=max;
    if(syshelp_scroll<0)syshelp_scroll=0;
    for(int i=0;i<visible&&i+syshelp_scroll<syshelp_count;i++){
        SDL_Texture *t=render_text_fit(ren,font_label,syshelp_lines[i+syshelp_scroll],g_ui_text,WIN_W-80);
        if(t){int w,h;SDL_QueryTexture(t,NULL,NULL,&w,&h);SDL_RenderCopy(ren,t,NULL,&(SDL_Rect){40,119+i*pitch,w,h});}
    }
    boot_text_block(ren,font_label,"Up/Down Scroll    B Back",th->accent2,(SDL_Rect){40,WIN_H-34,WIN_W-80,28});
}
#endif
