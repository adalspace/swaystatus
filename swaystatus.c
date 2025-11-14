#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BATTERY_PATH "/sys/class/power_supply/BAT0/capacity"

// ---- KEYBOARD LAYOUT ----
char *get_active_keyboard_layout() {
    FILE *fp = popen("swaymsg -t get_inputs | jq -r '.[] | select(.type==\"keyboard\") | .xkb_active_layout_name' | head -n1", "r");
    if (!fp) {
        return NULL;
    }

    static char buf[64];
    if (fgets(buf, sizeof(buf), fp)) {
        // strip newline
        buf[strcspn(buf, "\n")] = 0;
	pclose(fp);
	return buf;
    }

    pclose(fp);
    return NULL;
}

// ---- VOLUME MUTED ----
int is_volume_muted() {
    FILE *fp = popen("amixer get Master | grep -o '\\[on\\]\\|\\[off\\]' | head -n1", "r");
    if (!fp) return -1;

    char buf[8];
    if (!fgets(buf, sizeof(buf), fp)) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    if (strstr(buf, "[off]")) {
        return 1;  // muted
    } else if (strstr(buf, "[on]")) {
        return 0;  // not muted
    }
    return -1; // unknown
}

// ---- VOLUME LEVEL ----
int get_volume_percentage() {
    int current = -1;
    FILE *fp = popen("amixer sget Master | grep -o '[0-9]*%'", "r");
    if (fp == NULL) return -1;
    fscanf(fp, "%d%%\n", &current);
    pclose(fp);
    if (current == -1) return -1;
    return current;
}

// ---- BRIGHTNESS ----
int get_brightness_percentage() {
    int current = -1;
    int max = 1;
    FILE *fp = popen("brightnessctl g", "r");
    if (fp == NULL) return -1;
    fscanf(fp, "%d\n", &current);
    pclose(fp);
    if (current == -1) {
	return -1;
    }
    fp = popen("brightnessctl m", "r");
    if (fp == NULL) return -1;
    fscanf(fp, "%d\n", &max);
    pclose(fp);
    return (int)((float)current / (float)max * 100.0f);
}

// ---- BATTERY ----
int get_battery_percentage() {
    FILE *fp = fopen(BATTERY_PATH, "r");
    if (fp == NULL) {
        return -1;
    }
    int capacity;
    if (fscanf(fp, "%d", &capacity) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return capacity;
}

// ---- MEMORY ----
int get_memory_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return -1;

    long total = 0, available = 0;
    char label[64];
    long value;
    char unit[16];

    while (fscanf(fp, "%63s %ld %15s\n", label, &value, unit) == 3) {
        if (strcmp(label, "MemTotal:") == 0) total = value;
        if (strcmp(label, "MemAvailable:") == 0) {
            available = value;
            break;
        }
    }
    fclose(fp);

    if (total == 0) return -1;
    return (int)((100 * (total - available)) / total);
}

// ---- CPU ----
double get_cpu_usage() {
    static long prev_idle = 0, prev_total = 0;
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) return -1.0;

    char cpu[5];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(fp, "%4s %ld %ld %ld %ld %ld %ld %ld %ld",
           cpu, &user, &nice, &system, &idle,
           &iowait, &irq, &softirq, &steal);
    fclose(fp);

    long idle_time = idle + iowait;
    long total_time = user + nice + system + idle + iowait + irq + softirq + steal;

    long diff_idle = idle_time - prev_idle;
    long diff_total = total_time - prev_total;
    prev_idle = idle_time;
    prev_total = total_time;

    if (diff_total == 0) return 0.0;
    return (100.0 * (diff_total - diff_idle)) / diff_total;
}

// ---- WIFI ----
char *get_wifi_name() {
    static char ssid[64];
    FILE *fp = popen("nmcli -t -f NAME,DEVICE connection show --active | grep -m1 \"$(nmcli -t -f DEVICE device status | grep connected | cut -d: -f1)\" | cut -d: -f1 || echo \"No active connection\">/dev/null", "r");
    if (fp == NULL) return NULL;
    if (fgets(ssid, sizeof(ssid), fp) == NULL) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    // remove trailing newline
    ssid[strcspn(ssid, "\n")] = '\0';
    if (strcmp(ssid, "lo") == 0) {
	strcpy(ssid, "No Wi-Fi");
    }
    return ssid;
}

// ---- MAIN ----
int main() {
    // Print swaybar header
    printf("{\"version\":1}\n[\n");
    fflush(stdout);

    int first = 1;

    while (1) {
        // Time
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

	// Keyboard Layout
	char *keyboard_layout = get_active_keyboard_layout();
	
	// Brightness
	int current_brightness = get_brightness_percentage();

	// Volume
	int current_volume = get_volume_percentage();
	bool volume_muted = is_volume_muted() == 1;
	
        // Battery
        int battery = get_battery_percentage();
        const char *bat_color;
        if (battery < 0) {
            bat_color = "#AAAAAA";
        } else if (battery < 20) {
            bat_color = "#FF0000";
        } else if (battery < 50) {
            bat_color = "#FFFF00";
        } else {
            bat_color = "#00FF00";
        }

        // Memory
        int mem_used = get_memory_usage();

        // CPU
        double cpu_used = get_cpu_usage();

        // Wi-Fi
        char *wifi = get_wifi_name();
        if (!wifi || strlen(wifi) == 0) wifi = "No WiFi";

        // JSON output
        if (!first) {
            printf(",\n");
        }
        first = 0;

        printf("[");
        printf("{\"full_text\":\"   %.1f%% \",\"color\":\"#00FFFF\"},", cpu_used);
        printf("{\"full_text\":\"   %d%% \",\"color\":\"#FFA500\"},", mem_used);
        printf("{\"full_text\":\"   %s \",\"color\":\"#FFFFFF\"},", wifi);
        printf("{\"full_text\":\"   %s \",\"color\":\"#FFFFFF\"},", keyboard_layout);
        printf("{\"full_text\":\"   %d \",\"color\":\"#FFFFFF\"},", current_brightness);
        printf("{\"full_text\":\" ");
	if (volume_muted) printf("󰝟");
	else printf("󰕾");
	printf(" %d%% \",\"color\":\"#FFFFFF\"},", current_volume);
        printf("{\"full_text\":\"   %d%% \",\"color\":\"%s\"},", battery, bat_color);
        printf("{\"full_text\":\"   %s \",\"color\":\"#FFFFFF\"}", time_str);
        printf("]\n");
        fflush(stdout);

        sleep(1);
    }

    return 0;
}
