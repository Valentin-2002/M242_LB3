# M242_LB3

Benutzeranleitung

## Ausgangslage:

Code wurde auf das IOTKit gespielt und die WLAN - Zugangsdaten wurden im mbed.app.json angepasst
Mobile App "EasyMQTT" ist auf einem Mobiltelefon installiert.

## Schritt 1:

IOTKit ans Stromnetz anschliessen

## Schritt 2:

In EasyMQTT unter "Connect" mit dem Host cloud.tbz.ch und dem Port 1883 verbinden

## Schritt 3: 

In EasyMQTT unter dem Tab "Subscribe" die Topics "CKWLB3/mTemp" und "CKWLB3/wPos" abonnieren (Tipp: Topics zu Favoriten hinzufugen).
Die Messages vom IOTKit sind unter "Show Messages" sichtbar

## Schritt 4: 

In EasyMQTT unter dem Tab "Publish" unter dem Topic "CKWLB3/pTemp" die gewunschte Temparatur eingeben und senden
