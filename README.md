# ESPHome Dometic B2200

Externe ESPHome-Komponente zur IR-Steuerung einer Dometic-B2200-Dachklimaanlage über Home Assistant.

## Funktionen

- Ein/Aus
- Auto
- Kühlen von 16 bis 30 °C
- Heizen von 16 bis 30 °C
- Entfeuchten (Dry)
- Nur Lüfter
- Lüfterstufen `Stufe 1`, `Stufe 2`, `Stufe 3`, `Stufe 4`
- Licht
- Sleep
- I Feel mit optionalem Temperatursensor

## Installation

Kopiere `components/dometic_b2200` in dein ESPHome-Verzeichnis und ergänze:

```yaml
external_components:
  - source:
      type: local
      path: components
```

Eine vollständige Konfiguration befindet sich unter [`examples/m5stack_nano_c6.yaml`](examples/m5stack_nano_c6.yaml).

## Verwendung

```yaml
remote_transmitter:
  id: ir_tx
  pin: GPIO3
  carrier_duty_percent: 50%
  non_blocking: true

climate:
  - platform: dometic_b2200
    name: "Dometic B2200"
    transmitter_id: ir_tx
    light:
      name: "Dometic Licht"
    sleep:
      name: "Dometic Sleep"
    i_feel:
      name: "Dometic I Feel"
```

## Einschränkungen

- Die Verbindung ist unidirektional. Bedienungen mit der Originalfernbedienung werden nicht automatisch an Home Assistant zurückgemeldet.
- Die vier Lüfterstufen wurden im Modus „Nur Lüfter“ vermessen. Ob die Anlage dieselben Stufencodes in Cool/Heat separat auswertet, ist noch nicht bestätigt.
- I Feel wurde aus dem aufgenommenen Protokoll rekonstruiert. Für die sinnvolle Nutzung sollte `temperature_sensor` gesetzt werden.
- Die Tail-Bytes `3B C4 53 AC` wurden direkt gegen ein funktionierendes Originaltelegramm verifiziert. `tail_byte_1` und `tail_byte_2` bleiben bei Bedarf konfigurierbar.

## Protokoll

Die Reverse-Engineering-Dokumentation befindet sich in [`PROTOCOL.md`](PROTOCOL.md).

## Lizenz

MIT
