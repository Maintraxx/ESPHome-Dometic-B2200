# Dometic B2200 IR protocol

## Physikalische Codierung

| Eigenschaft | Wert |
|---|---:|
| Trägerfrequenz | 38 kHz |
| Header | Mark 6250 µs, Space 7200 µs |
| Bit-Mark | 630 µs |
| Bit 0 | Space 1250 µs |
| Bit 1 | Space 3130 µs |
| Reihenfolge | MSB first |
| Nutzdaten | 64 Bit / 8 Byte |
| Abschluss | Mark 630 µs, Space 7250 µs, Mark 630 µs |

Der demodulierende IR-Empfänger stellt Mark/Space im Rohlog elektrisch invertiert dar. Der Sender verwendet die oben genannten Mark-/Space-Werte.

## Frame

```text
B0  ~B0  B2  ~B2  B4  ~B4  B6  ~B6
```

Jedes Nutzbyte wird unmittelbar von seinem bitweisen Komplement gefolgt.

### Byte 0: Statusbits

| Bit | Maske | Bedeutung |
|---:|---:|---|
| 7 | `0x80` | Power: 1 = an, 0 = aus |
| 6 | `0x40` | Licht |
| 3 | `0x08` | Sleep |
| 1 | `0x02` | im Dry-Telegramm gelöscht |
| 0 | `0x01` | I Feel, active-low |

Normaler Grundwert: `0x87`.

### Byte 2: Befehl

#### Kühlen

```text
command = ((temperature - 16) << 4) | 0x09
```

Beispiele: 20 °C = `0x49`, 21 °C = `0x59`, 22 °C = `0x69`, 23 °C = `0x79`.

#### Heizen

```text
command = ((temperature - 16) << 4) | 0x02
```

Beispiele: 20 °C = `0x42`, 21 °C = `0x52`, 22 °C = `0x62`.

#### Nur Lüfter

| Stufe | Code |
|---|---:|
| 1 | `0x63` |
| 2 | `0x67` |
| 3 | `0x6B` |
| 4 | `0x6F` |

#### Weitere Modi

| Modus | Code |
|---|---:|
| Auto Change | `0x40` |
| Dry | `0x30` |

### Bytes 4 bis 7

Verifizierter Tail:

```text
3B C4 53 AC
```

Die Komponente speichert `0x3B` und `0x53`; die Komplementbytes werden automatisch gebildet.
