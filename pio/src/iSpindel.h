extern MPUOffset offset;
extern int16_t ax, ay, az;
extern float Volt, Temperatur, Tilt, Gravity;

// Definicje stałych dla interfejsu użytkownika
extern const char HTTP_API_LIST[];
extern const char HTTP_TEMPSCALE_LIST[];
#define TYPE_HIDDEN "type=\"hidden\""
#define TYPE_NUMBER "type=\"number\" min=\"0\" step=\"1\""
#define TYPE_CHECKBOX "type=\"checkbox\""
#define TYPE_CHECKBOX_CHECKED "type=\"checkbox\" checked=\"checked\"" 