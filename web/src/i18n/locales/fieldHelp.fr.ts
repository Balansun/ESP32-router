/** Aide inline détaillée (icône ? dépliable) — Français. */
export const fieldHelpFr = {
  settings: {
    install_country:
      "Le pays prédéfini fixe la tension et la fréquence secteur par défaut pour la régulation et l’interface. N’utilisez Personnalisé que si votre réseau diffère des profils listés.",
    install_country_variant:
      "Certains pays ont des réseaux scindés (ex. Japon 50/60 Hz, Brésil 127/220 V). Choisissez la variante correspondant à votre installation.",
    mains_nominal_v:
      "Tension nominale phase-neutre pour les calculs de puissance. UE typique : 230 V ; Amérique du Nord souvent 120/240 V selon le câblage.",
    mains_frequency_mode:
      "Auto lit la fréquence sur le compteur actif si disponible. Manuel impose 50 ou 60 Hz quand la source ne la fournit pas.",
    mains_frequency_hz_manual: "Fréquence de repli (50 ou 60 Hz) en mode Manuel.",
    edit_mains_defaults:
      "Permet de modifier tension/fréquence par défaut du pays. À n’utiliser que si votre réseau local diffère du tableau.",
    router_name:
      "Nom convivial sur le LAN et dans la découverte MQTT. Utilisé aussi en mDNS : http://{nom}.local si le réseau le permet.",
    probe_house_name:
      "Libellé du canal « maison » dans l’UI et MQTT (ex. « Réseau », « Maison »). Ne change pas le câblage.",
    probe_second_name:
      "Libellé du second canal si la source le fournit (ex. charge routée, CH2 sur modules JSY).",
    temperature_label:
      "Nom affiché pour la sonde DS18B20 sur GPIO13. Sonde 1-Wire optionnelle pour ballon ou ambiante.",
    dhcp:
      "L’ESP32 demande une adresse IP à la box (DHCP). Désactivez seulement pour une IP fixe sur le LAN.",
    ip_fixed:
      "Adresse IPv4 statique si DHCP désactivé. Doit être libre sur le sous-réseau. Nécessite un redémarrage.",
    gateway:
      "Passerelle par défaut (souvent la box). Requise en IP fixe pour MQTT, NTP et compteurs HTTP.",
    subnet_mask:
      "Masque de sous-réseau (souvent 255.255.255.0). Doit correspondre à votre LAN.",
    dns:
      "Serveur DNS en IP fixe (souvent la passerelle ou 1.1.1.1). Utilisé pour NTP et pairs HTTP.",
    mqtt_repeat_sec:
      "Intervalle de publication MQTT (secondes). 0 = désactive toute publication tout en conservant les paramètres broker.",
    mqtt_ip:
      "Adresse IPv4 du broker MQTT (add-on Home Assistant, Mosquitto…). Doit être joignable depuis l’ESP32.",
    mqtt_port:
      "Port TCP MQTT (1883 par défaut). TLS souvent sur 8883 (support TLS selon build).",
    mqtt_user:
      "Utilisateur MQTT si le broker l’exige. Laisser vide uniquement sur LAN de confiance.",
    mqtt_password:
      "Mot de passe MQTT associé. Stocké en EEPROM ; protégez l’accès au réseau local.",
    mqtt_prefix:
      "Préfixe des topics d’état et commandes (ex. balansun). La découverte HA utilise toujours homeassistant/.",
    mqtt_device_name:
      "Identifiant court dans les topics (ex. 6809475d1df8). Par défaut : device_uid de l’appareil (MAC usine, 12 chiffres hex). Pas d’espaces.",
    vacation_enabled:
      "Suspend le routage d’excédent (triac et actions en politique « vacances »). Utile en absence ou maintenance.",
    vacation_end_epoch:
      "Date et heure de reprise automatique du routage (fuseau de l’appareil). Laisser vide pour garder les vacances jusqu’à désactivation manuelle.",
    triac_off_when_source_stale:
      "Fail-safe de routage : si activé et que la source de mesure est « périmée » (score santé < 50), le firmware suspend les sorties (triac 0 %, actions GPIO/HTTP locales arrêtées) via `output_suspend.reason: source_stale`. Reprise automatique dès qu’une mesure valide revient — pas de verrouillage persistant (contrairement à `safety_lockout`). Recommandé pour BalansunPeer (install scindée), compteurs HTTP et Pmqtt. Le capteur MQTT `source_stale` signale l’état ; cet interrupteur décide si le triac doit être coupé. Ne pas confondre avec `triac_backoff_when_heater_idle` (retour CH2 ~45 s).",
    triac_backoff_when_heater_idle:
      "Avec JSY-MK-194 et CH2 sur la charge routée : coupe le triac si commande active mais CH2 ≈ 0 pendant ~45 s (thermostat en série ouvert). Câbler CH2 sur la branche de la charge routée (après le triac).",
    load_profile:
      "Type de charge qui reçoit l’excédent (chauffe-eau triac ou profil triac générique).",
    max_routed_w:
      "Plafond de puissance routée sur le site (W). 0 = désactivé. Limite la diversion totale si besoin de marge.",
    mqtt_json_commands:
      "Active l’abonnement MQTT action_N/config/set (JSON schéma v1) pour configurer les actions depuis HA ou scripts.",
    calib_u:
      "Coefficient de tension pour entrées analogiques. Défaut 1000 ; augmenter si la lecture est basse vs référence.",
    calib_i:
      "Coefficient de courant analogique. Défaut 1000 ; ajuster après pose des TC et comparaison compteur.",
    pmqtt_topic:
      "Topic MQTT abonné en source Pmqtt. Payload JSON ; schéma selon le preset ci-dessous. Même broker que la télémétrie.",
    pmqtt_preset:
      "Pw / Pf simple : {\"Pw\":-1500,\"Pf\":0.98}. Maison : objet imbriqué active_import_w / active_export_w. Personnalisé : clés manuelles.",
    pmqtt_schema_custom:
      "Chemins de clés JSON séparés par des virgules (avancé). Voir guide § A.7 et fixtures firmware.",
    triac_override_max_temp_c:
      "Bloque le forçage triac 100 % si la DS18B20 dépasse cette température (°C). 0 = désactivé. 40–120 si actif.",
    jsy_mk333_serial_baud:
      "Débit UART pour JSY-MK-333 sur UART2. Défaut 9600 ; doit correspondre au compteur (1200–115200).",
    pin_triac_dim:
      "Sortie DIM / gate triac (souvent GPIO 22 sur WROOM). Appliqué après redémarrage.",
    pin_zero_cross:
      "Entrée passage par zéro pour découpage de phase (souvent GPIO 23). Appliqué après redémarrage.",
    pin_uart_rx: "RX UART compteur (JSY / Linky). Appliqué après redémarrage.",
    pin_uart_tx: "TX UART compteur. Doit être différent de RX. Appliqué après redémarrage.",
    pin_temp:
      "Bus 1-Wire DS18B20 optionnel. -1 = sonde désactivée ; sinon numéro GPIO. Appliqué après redémarrage.",
    pin_analog0: "Entrée analogique sonde analogique 0.",
    pin_analog1: "Entrée analogique sonde analogique 1.",
    pin_analog2: "Entrée analogique sonde analogique 2.",
    pwm_gpio:
      "GPIO pour sortie PWM SSR CC (-1 = off). Broches autorisées : 4, 5, 14, 16, 17, 21, 25. Driver matériel requis.",
    pwm_mode:
      "off : désactivé. follow_triac : suit le triac. independent : utilise pwm_duty_percent indépendamment.",
    pwm_duty_percent: "Rapport cyclique 0–100 % si pwm_mode = independent.",
    pwm_inverted: "Inverse la polarité PWM pour modules SSR actifs à l’état bas.",
    status_led_mode:
      "off : pas de LED. dual_gpio : deux sorties GPIO (jaune/vert Balansun). rgb_ws2812 : une LED WS2812 (onboard S3 ou GPIO).",
    status_led_gpio_activity:
      "GPIO clignotement Wi‑Fi / activité. Défaut 18 sur PCB Balansun. -1 désactive cette sortie.",
    status_led_gpio_regulation:
      "GPIO charge triac. Défaut 19 sur PCB Balansun. -1 désactive cette sortie.",
    status_led_rgb_gpio:
      "Broche WS2812. -1 = LED RGB onboard S3 N16R8 (GPIO 48 / RGB_BUILTIN ; souder le pad RGB si besoin). Mettre 48 si -1 ne répond pas.",
    status_led_active_low:
      "Cochez si vos LED GPIO s’allument au niveau bas (mode dual_gpio uniquement).",
    status_led_color_activity:
      "Couleur RGB [r,g,b] pour l’activité / Wi‑Fi en mode rgb_ws2812.",
    status_led_color_regulation:
      "Couleur RGB [r,g,b] pour la régulation triac en mode rgb_ws2812.",
    status_led_color_reboot:
      "Superposition RGB pendant un redémarrage planifié (rouge par défaut). En deux GPIO, les deux LED clignotent ensemble.",
    status_led_color_ap:
      "Superposition RGB en mode point d’accès / portail captif (violet par défaut [102,0,255]). En deux GPIO, les deux sorties clignotent.",
    tempo_rte_enabled:
      "Si la source n’est pas Linky, récupère les couleurs Tempo EDF via api-couleur-tempo.fr au lieu de la TIC Linky.",
    tz_country:
      "Région pour le fuseau IANA : plannings, historiques et fin de vacances.",
    time_ntp1: "Serveur NTP principal (nom d’hôte ou IP). Requis pour horloge et horodatages MQTT.",
    time_ntp2: "Serveur NTP secondaire si le premier est injoignable.",
    http_cors_enabled:
      "Autorise GET /api/v1 cross-origin et preflight OPTIONS (Try it out docs avec l’IP du routeur). Labo uniquement ; lectures GET — pas de connexion cross-origin. Pas sur réseau exposé Internet.",
    http_auth_enabled:
      "Exige une session sur /api/v1. Le jeton de session n’est stocké que dans cet onglet navigateur.",
    http_auth_password:
      "Mot de passe API hashé sur l’appareil. En point d’accès, /wifi et restauration sauvegarde restent ouverts pour récupération.",
    factory_reset:
      "Efface EEPROM et historique puis redémarre. Exportez une sauvegarde avant ; irréversible.",
  },

  api: {
    http_auth_password:
      "Mot de passe API hashé sur l’appareil. En point d’accès, /wifi et restauration restent ouverts. Le changer révoque tous les jetons permanents.",
    http_cors_enabled:
      "Autorise GET /api/v1 cross-origin et preflight OPTIONS. Labo uniquement ; pas sur réseau exposé Internet.",
    api_access_token:
      "Jeton Bearer durable pour automatisations (curl, Home Assistant). Affiché une seule fois à la création ; le secret est stocké sur l’appareil et inclus dans l’export Réglages → Sauvegarde.",
  },

  actions: {
    sensitivity:
      "Réactivité du triac face à l’excédent. Bas = stable mais lent ; haut = rapide mais risque d’oscillation. Valeur 1–100.",
    host:
      "Cible HTTP : IP/nom d’hôte, ou localhost pour un GPIO sur cet ESP32 (voir format dans l’indication courte).",
    port: "Port TCP pour actions HTTP (80 par défaut). Ignoré pour GPIO localhost.",
    path_on:
      "Chemin ou requête pour allumer. Ex. distant : /rpc/Switch.Set?id=0&on=true. GPIO local : gpio=5&out=1",
    path_off: "Commande d’extinction. Peut différer de path_on.",
    repeat_sec:
      "Répète la commande toutes les N secondes tant que l’état doit tenir (0 = une seule fois). Utile si le pair HTTP est capricieux.",
    tempo_sec:
      "Délai minimum entre deux changements d’état HTTP pour ne pas surcharger le périphérique.",
    edit_mode:
      "Mode de créneau : forcé off/on, routage par seuils sur Pw, ou routage triac. Dernier créneau jusqu’à minuit.",
    edit_threshold:
      "Seuil de puissance maison (W) en mode puissance. Convention : négatif souvent = injection/excédent.",
    edit_max_open: "Ouverture triac max (%) autorisée sur ce créneau.",
    edit_power_on: "Mode puissance : allumer si Pw en dessous de cette valeur (W).",
    edit_power_off: "Mode puissance : éteindre si Pw au-dessus de cette valeur (W).",
    edit_temp_inf:
      "Borne basse optionnelle (°C) avec DS18B20. Vide (128) = pas de condition température.",
    edit_temp_sup:
      "Borne haute optionnelle (°C). Sécurité : éviter 100 % si sonde au-dessus du plafond réglages.",
    edit_hour_end: "Heure de fin (0–24) ; le créneau suivant commence à cette heure.",
    action_daily_cap_wh:
      "Énergie max (Wh) routée par le triac par jour civil, comptée via l’export journalier CH2 quand disponible. 0 = pas de plafond. Remise à zéro à minuit (heure locale appareil).",
  },

  firmware: {
    fw_file:
      "Binaire firmware (.bin) pour ESP32-WROOM-32 Balansun. Mauvaise cible ou partition peut rendre l’appareil inutilisable.",
    fw_md5:
      "Somme MD5 optionnelle vérifiée avant redémarrage OTA. Recommandé si le fichier a transité par le réseau.",
    ota_new: "Nouveau mot de passe pour upload OTA Arduino/PlatformIO (sécurité optionnelle).",
    ota_confirm: "Confirmez le mot de passe OTA pour éviter les fautes de frappe.",
  },

  wifi: {
    wifi_ssid:
      "Nom du réseau Wi‑Fi (SSID) en mode station. En configuration point d’accès, utilisez la recherche des réseaux.",
    wifi_password:
      "Phrase secrète WPA. Laisser vide seulement pour réseaux ouverts (déconseillé).",
  },

  sourceWizard: {
    ext_peer_ip:
      "IPv4 de l’ESP distant ou du compteur exposant GET /api/v1/measurements. Même LAN requis.",
    ext_peer_port: "Port TCP (80 par défaut). Si écoute non standard ou proxy.",
    ext_peer_path:
      "Chemin HTTP commençant par / (défaut /api/v1/measurements). Max 48 caractères.",
    enphase_user: "Utilisateur API locale Enphase Envoy si requis par la passerelle.",
    enphase_password: "Mot de passe Enphase pour accès HTTP/HTTPS local.",
    enphase_serial:
      "Numéro de série ou canal optionnel (ex. Shelly Pro 3EM total = 3). Voir guide source.",
    pmqtt_topic: "Topic broker pour JSON puissance en source Pmqtt.",
    jsy_mk333_serial_baud: "Débit JSY-MK-333 ; doit correspondre au compteur (défaut 9600).",
    calib_u: "Coefficient tension Analog (défaut 1000).",
    calib_i: "Coefficient courant Analog (défaut 1000).",
    pmqtt_preset: "Format JSON publié par votre compteur externe ou intégration Home Assistant.",
    defer_test:
      "Ignore le test diagnostics à l’enregistrement. Si le pair est hors ligne ; vérifiez les mesures ensuite.",
  },

  httpAuth: {
    login_password:
      "Mot de passe API défini dans Paramètres → Accès HTTP. Connectez-vous sur la page de connexion pour obtenir un jeton de session.",
  },

  install: {
    install_country:
      "Le pays prédéfini fixe tension et fréquence secteur par défaut. Même champ que Paramètres ; choisi tôt à la première installation.",
  },

  backup: {
    sectionSecurity:
      "La sauvegarde contient Wi‑Fi, MQTT et URLs d’actions. Stockez-la de façon sécurisée, comme un export de mots de passe.",
    sectionExport:
      "Télécharge la configuration complète en JSON. À faire avant mise à jour firmware qui change l’EEPROM.",
    sectionImport:
      "Restaure une sauvegarde complète. Redémarrage possible ; reconnectez-vous sur le LAN.",
  },
} as const;
