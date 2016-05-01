#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include "json/json.h"

Json::Value jdata;
Json::Value jtmp;
bool HumanReadAble = false;
bool HasOutputFile = false;
std::string SourceFile, OutputFile, BinaryFile;

static std::vector<std::pair<std::string, std::string> > CommandLineArguments;

static void ParseCommandLineArguments(int ac, char **av)
{
	for (int i = 1; i < ac; ++i)
	{
		std::string option = av[i];
		std::string param;
		while (!option.empty() && option[0] == '-')
			option.erase(option.begin());
		size_t t = option.find('=');
		if (t != std::string::npos)
		{
			param = option.substr(t + 1);
			option.erase(t);
		}
		if (option.empty())
			continue;
		CommandLineArguments.push_back(std::make_pair(option, param));
	}
}

static bool GetCommandLineArgument(const std::string &name, char shortname, std::string &param)
{
	std::string tmp;
	param.clear();
	for (std::vector<std::pair<std::string, std::string> >::iterator it = CommandLineArguments.begin(), it_end = CommandLineArguments.end(); it != it_end; ++it)
	{
		if ((it->first.compare(name) == 0) || (it->first.length() == 1 && it->first[0] == shortname))
		{
			param = it->second;
			return true;
		}
	}
	return false;
}

static bool GetCommandLineArgument(const std::string &name, char shortname = 0)
{
	std::string Unused;
	return GetCommandLineArgument(name, shortname, Unused);
}


enum Listtypes
{
	Wettkampfdefinitionsliste,
	Vereinsmeldeliste,
	Vereinsergebnisliste,
	Wettkampfergebnisliste,
	undefiniert
};

Listtypes Listart = undefiniert;

std::vector<std::string> splitline(std::string &line)
{
	std::vector<std::string> split;
	std::istringstream ss(line);
	std::string token;

	std::getline(ss, token, ':');
	split.push_back(token);
	while(std::getline(ss, token, ';'))
	{
		split.push_back(token);
	}
	return split;
}





static void Syntax()
{
	std::cout << "Aufruf: " << BinaryFile << " QUELLDATEI [ZIELDATEI]" << std::endl;
	std::cout << "Wenn keine Datei für den Output angegeben wird," << std::endl;
	std::cout << "erfolgt die Ausgabe im Terminal." << std::endl;
	// -h, --human = human readable
	// -s, --source = source file mit parameter
	// -o, --output = output file mit parameter
}


void debug(const std::string &line, const std::vector<std::string> &list)
{
	std::cout << line << std::endl;
	for (size_t i = 0; i < list.size(); i++)
		std::cout << i << ": " << list[i] << std::endl;
}

void addj(const std::string &type, const std::string &attr, const std::vector<std::string> &list, size_t item)
{
		if (!list[item].empty())
			jdata[type][attr] = list[item];
}

void addjtmp(const std::string &attr, const std::vector<std::string> &list, size_t item)
{
		if (!list[item].empty())
			jtmp[attr] = list[item];
}

// return values
//   0 = ok
//   1 = error (return value = zeilennummer in der der Fehler auftrat)

size_t lese(std::ifstream &in)
{
	std::vector<std::string> list;
	std::string line;
	size_t line_number = 0;

	while (getline(in, line))
	{
		line_number++;
		if(!line.empty() && *line.rbegin() == '\r')
		{
			// wir löschen das windows linefeed am Ende jeder Zeile. (CR/LF)
			line.erase( line.length()-1, 1);
		}
		if (line.empty())
		{
			// hurra, eine leere zeile gefunden
			// wir beenden diesen Schleifendurchgang
			// und lesen gleich die nächste Zeile ein
			continue;
		}
		else if ((line.find("(*")!= std::string::npos) && (line.find("*)") != std::string::npos))
		{
			// hurra, diese Zeile enthält einen Kommentar
			// wir beenden diesen Schleifendurchgang
			// und lesen gleich die nächste Zeile ein
			continue;
		}
		list = splitline(line);
		if (list.size() < 1)
			return line_number;
		if (list[0] == "FORMAT")
		{
			if (list.size() < 3)
				return line_number;
			addj("FORMAT","Listart", list,1);
			addj("FORMAT","Version", list,2);

			if (list[1] == "Wettkampfdefinitionsliste")
				Listart = Wettkampfdefinitionsliste;
			else if (list[1] == "Vereinsmeldeliste")
				Listart = Vereinsmeldeliste;
			else if (list[1] == "Vereinsergebnisliste")
				Listart = Vereinsergebnisliste;
			else if (list[1] == "Wettkampfergebnisliste")
				Listart = Wettkampfergebnisliste;
		}
		else if (list[0] == "ERZEUGER")
		{
			if (list.size() < 4)
				return line_number;
			addj("ERZEUGER","Software", list,1);
			addj("ERZEUGER","Version", list,2);
			addj("ERZEUGER","Kontakt", list,3);
		}
		else if (list[0] == "VERANSTALTUNG")
		{
			if (list.size() < 5)
				return line_number;
			addj("VERANSTALTUNG","Veranstaltungsbezeichnung", list,1);
			addj("VERANSTALTUNG","Veranstaltungsort", list,2);
			addj("VERANSTALTUNG","Bahnlänge", list,3);
			addj("VERANSTALTUNG","Zeitmessung", list,4);
		}
		else if (list[0] == "VERANSTALTUNGSORT")
		{
			if (list.size() < 9)
				return line_number;
			addj("VERANSTALTUNGSORT","Name Schwimmhalle", list, 1);
			addj("VERANSTALTUNGSORT","Straße", list, 2);
			addj("VERANSTALTUNGSORT","PLZ", list, 3);
			addj("VERANSTALTUNGSORT","Ort", list, 4);
			addj("VERANSTALTUNGSORT","Land", list, 5);
			addj("VERANSTALTUNGSORT","Telefon", list, 6);
			addj("VERANSTALTUNGSORT","Fax", list, 7);
			addj("VERANSTALTUNGSORT","eMail", list, 8);
		}
		else if (list[0] == "AUSSCHREIBUNGIMNETZ")
		{
			if (list.size() < 2)
				return line_number;
			addj("AUSSCHREIBUNGIMNETZ","Internetadresse", list,1);
		}
		else if (list[0] == "VERANSTALTER")
		{
			if (list.size() < 2)
				return line_number;
			addj("VERANSTALTER","Name des Veranstalters", list,1);
		}
		else if (list[0] == "AUSRICHTER")
		{
			if (list.size() < 10)
				return line_number;
			addj("AUSRICHTER","Name des Ausrichters", list, 1);
			addj("AUSRICHTER","Name", list, 2);
			addj("AUSRICHTER","Straße", list, 3);
			addj("AUSRICHTER","PLZ", list, 4);
			addj("AUSRICHTER","Ort", list, 5);
			addj("AUSRICHTER","Land", list, 6);
			addj("AUSRICHTER","Telefon", list, 7);
			addj("AUSRICHTER","Fax", list, 8);
			addj("AUSRICHTER","eMail", list, 9);
		}
		else if (list[0] == "MELDEADRESSE")
		{
			if (list.size() < 9)
				return line_number;
			jdata["MELDEADRESSE"]["Name"] = list[1];
			jdata["MELDEADRESSE"]["Straße"] = list[2];
			jdata["MELDEADRESSE"]["PLZ"] = list[3];
			jdata["MELDEADRESSE"]["Ort"] = list[4];
			jdata["MELDEADRESSE"]["Land"] = list[5];
			jdata["MELDEADRESSE"]["Telefon"] = list[6];
			jdata["MELDEADRESSE"]["Fax"] = list[7];
			jdata["MELDEADRESSE"]["eMail"] = list[8];
		}
		else if (list[0] == "MELDESCHLUSS")
		{
			if (list.size() < 3)
				return line_number;
			jdata["MELDESCHLUSS"]["Datum"] = list[1];
			jdata["MELDESCHLUSS"]["Uhrzeit"] = list[2];
		}
		else if (list[0] == "BANKVERBINDUNG")
		{
			if (list.size() < 4)
				return line_number;
			jdata["MELDESCHLUSS"]["Name der Bank"] = list[1];
			jdata["MELDESCHLUSS"]["IBAN"] = list[2];
			jdata["MELDESCHLUSS"]["BIC"] = list[3];
		}
		else if (list[0] == "BESONDERES")
		{
			if (list.size() < 2)
				return line_number;
			jdata["BESONDERES"]["Anmerkungen"] = list[1];
		}
		else if (list[0] == "NACHWEIS")
		{
			if (Listart == Wettkampfdefinitionsliste)
			{
				if (list.size() < 4)
					return line_number;
				jdata["NACHWEIS"]["Nachweis von"] = list[1];
				jdata["NACHWEIS"]["Nachweis bis"] = list[2];
				jdata["NACHWEIS"]["Bahnlänge"] = list[3];
			}
		}
		else if (list[0] == "ABSCHNITT")  // das kann öfters vorkommen
		{
			jtmp.clear();
			if (Listart == Wettkampfdefinitionsliste)
			{
				if (list.size() < 6)
					return line_number;
				addjtmp("Abschnittsnr.", list, 1);
				addjtmp("Abschnittsdatum", list, 2);
				addjtmp("Einlass", list, 3);
				addjtmp("Kampfrichtersitzung", list, 4);
				addjtmp("Anfangszeit", list, 5);
				addjtmp("Relative Angabe", list, 6);
			}
			else  // das Format ist bei den anderen drei Listen gleich
			{
				if (list.size() < 5)
					return line_number;
				addjtmp("Abschnittsnr.", list, 1);
				addjtmp("Abschnittsdatum", list, 2);
				addjtmp("Anfangszeit", list, 3);
				addjtmp("Relative Angabe", list, 4);
			}
			jdata["ABSCHNITT"].append(jtmp);
		}
		else if (list[0] == "WETTKAMPF") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 12)
				return line_number;
			addjtmp("Wettkampfnr.", list, 1);
			addjtmp("Wettkampfart", list, 2);
			addjtmp("Abschnittsnr.", list, 3);
			addjtmp("Anzahl Starter", list, 4);
			addjtmp("Einzelstrecke", list, 5);
			addjtmp("Technik", list, 6);
			addjtmp("Ausübung", list, 7);
			addjtmp("Geschlecht", list, 8);
			addjtmp("Zuordnung Bestenliste", list, 9);
			addjtmp("Qualifikationswettkampfnr", list, 10);
			addjtmp("Qualifikationswettkampfart", list, 11);
			jdata["WETTKAMPF"].append(jtmp);
		}
		else if (list[0] == "WERTUNG") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 9)
				return line_number;
			addjtmp("Wettkampfnr.", list, 1);
			addjtmp("Wettkampfart", list, 2);
			addjtmp("WertungsID", list, 3);
			addjtmp("Wertungsklasse Typ", list, 4);
			addjtmp("Mindest-Jahrgang/Altersklasse", list, 5);
			addjtmp("Maximale Jahrgang/Altersklasse", list, 6);
			addjtmp("Geschlecht", list, 7);
			addjtmp("Wertungsname", list, 8);
			jdata["WERTUNG"].append(jtmp);
		}
		else if (list[0] == "PFLICHTZEIT") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 8)
				return line_number;
			addjtmp("Wettkampfnr.", list, 1);
			addjtmp("Wettkampfart", list, 2);
			addjtmp("Wertungsklasse Typ", list, 3);
			addjtmp("Mindes-Jahrgang/Altersklasse", list, 4);
			addjtmp("Maximale Jahrgang/Altersklasse", list, 5);
			addjtmp("Pflichtzeit", list, 6);
			addjtmp("Geschlecht", list, 7);
			jdata["PFLICHTZEIT"].append(jtmp);
		}
		else if (list[0] == "MELDEGELD") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 4)
				return line_number;
			addjtmp("Meldegeld Typ", list, 1);
			addjtmp("Betrag", list, 2);
			addjtmp("Wettkampfnr", list, 3);
			jdata["MELDEGELD"].append(jtmp);
		}
		else if (list[0] == "VEREIN")
		{
			if (list.size() < 5)
				return line_number;
			if (Listart == Wettkampfergebnisliste)  // kann öfters vorkommen
			{
				jtmp.clear();
				addjtmp("Vereinsbezeichnung", list, 1);
				addjtmp("Vereinskennzahl", list, 2);
				addjtmp("Landesschwimmverband", list, 3);
				addjtmp("Fina-Nationenkürzel", list, 4);
				jdata["VEREIN"].append(jtmp);
			}
			else		// kommt nur einmal vor  (Vereinsmeldliste + Vereinsergebnisliste)
			{
				addj("VEREIN","Vereinsbezeichnung", list, 1);
				addj("VEREIN","Vereinskennzahl", list, 2);
				addj("VEREIN","Landesschwimmverband", list, 3);
				addj("VEREIN","Fina-Nationenkürzel", list, 4);
			}
		}
		else if (list[0] == "ANSPRECHPARTNER")
		{
			if (list.size() < 9)
				return line_number;
			addj("ANSPRECHPARTNER","Name", list, 1);
			addj("ANSPRECHPARTNER","Straße", list, 2);
			addj("ANSPRECHPARTNER","PLZ", list, 3);
			addj("ANSPRECHPARTNER","Ort", list, 4);
			addj("ANSPRECHPARTNER","Land", list, 5);
			addj("ANSPRECHPARTNER","Telefon", list, 6);
			addj("ANSPRECHPARTNER","Fax", list, 7);
			addj("ANSPRECHPARTNER","eMail", list, 8);
		}
		else if (list[0] == "KARIMELDUNG") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 4)
				return line_number;
			addjtmp("Nummer Kampfrichter", list, 1);
			addjtmp("Name", list, 2);
			addjtmp("Kampfrichtergruppe", list, 3);
			jdata["KARIMELDUNG"].append(jtmp);
		}
		else if (list[0] == "KARIABSCHNITT") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 4)
				return line_number;
			addjtmp("Nummer Kampfrichter", list, 1);
			addjtmp("Abschnittsnummer", list, 2);
			addjtmp("Einsatzwunsch", list, 3);
			jdata["KARIABSCHNITT"].append(jtmp);
		}
		else if (list[0] == "TRAINER") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 3)
				return line_number;
			addjtmp("Nummer Trainer", list, 1);
			addjtmp("Name", list, 2);
			jdata["TRAINER"].append(jtmp);
		}
		else if (list[0] == "PNMELDUNG") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 8)
				return line_number;
			addjtmp("Name", list, 1);
			addjtmp("DSV-ID Schwimmer", list, 2);
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 3);
			addjtmp("Geschlecht des Schwimmers", list, 4);
			addjtmp("Jahrgang", list, 5);
			addjtmp("Altersklasse", list, 6);
			addjtmp("Nummer Trainer", list, 7);
			jdata["PNMELDUNG"].append(jtmp);
		}
		else if (list[0] == "STARTPN") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 4)
				return line_number;
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 1);
			addjtmp("Wettkampfnummer", list, 2);
			addjtmp("Meldezeit", list, 3);
			jdata["STARTPN"].append(jtmp);
		}
		else if (list[0] == "STMELDUNG") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 7)
				return line_number;
			addjtmp("Nummer der Mannschaft", list, 1);
			addjtmp("Veranstaltungs-ID der Staffel", list, 2);
			addjtmp("Wertungsklasse Typ", list, 3);
			addjtmp("Mindest-Jahrgang/Altersklasse", list, 4);
			addjtmp("Maximale Jahrgang/Altersklasse", list, 5);
			addjtmp("Name der Staffel", list, 6);
			jdata["STMELDUNG"].append(jtmp);
		}
		else if (list[0] == "STARTST") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 4)
				return line_number;
			addjtmp("Veranstaltungs-ID der Staffel", list, 1);
			addjtmp("Wettkampfnummer", list, 2);
			addjtmp("Meldezeit", list, 3);
			jdata["STARTST"].append(jtmp);
		}
		else if (list[0] == "STAFFELPERSON") // das kann öfters vorkommen
		{
			jtmp.clear();
			if ((Listart == Vereinsergebnisliste) || (Listart == Wettkampfergebnisliste))
			{
				if (list.size() < 10)
					return line_number;
				addjtmp("Veranstaltungs-ID der Staffel", list, 1);
				addjtmp("Wettkampfnummer", list, 2);
				addjtmp("Wettkampfart", list, 3);
				addjtmp("Name", list, 4);
				addjtmp("DSV-ID des Schwimmers", list, 5);
				addjtmp("Startnummer des Schwimmers innerhalb der Staffel", list, 6);
				addjtmp("Geschlecht des Schwimmers", list, 7);
				addjtmp("Jahrgang", list, 8);
				addjtmp("Altersklasse", list, 9);
			}
			else
			{
				if (list.size() < 5)
					return line_number;
				addjtmp("Veranstaltungs-ID der Staffel", list, 1);
				addjtmp("Wettkampfnummer", list, 2);
				addjtmp("Veranstaltungs-ID des Schwimmers", list, 3);
				addjtmp("Startnummer des Schwimmers innerhalb der Staffel", list, 4);
			}
			jdata["STAFFELPERSON"].append(jtmp);
		}
		else if (list[0] == "KAMPFGERICHT") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 5)
				return line_number;
			addjtmp("Abschnittsnr.", list, 1);
			addjtmp("Position", list, 2);
			addjtmp("Name des Kampfrichter", list, 3);
			addjtmp("Verein des Kampfrichters", list, 4);
			jdata["KAMPFGERICHT"].append(jtmp);
		}
		else if (list[0] == "PERSON") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 6)
				return line_number;
			addjtmp("DSV-ID Schwimmer", list, 1);
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 2);
			addjtmp("Geschlecht des Schwimmers", list, 3);
			addjtmp("Jahrgang", list, 4);
			addjtmp("Altersklasse", list, 5);
			jdata["PERSON"].append(jtmp);
		}
		else if (list[0] == "PERSONENERGEBNIS") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 10)
				return line_number;
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("WertungsID", list, 4);
			addjtmp("Platz", list, 5);
			addjtmp("Endzeit", list, 6);
			addjtmp("Grund der Nichtwertung", list, 7);
			addjtmp("Disqualifikationsbemerkung", list, 8);
			addjtmp("Erhöhtes nachträgliches Meldegeld", list, 9);
			jdata["PERSONENERGEBNIS"].append(jtmp);
		}
		else if (list[0] == "PNZWISCHENZEIT") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 6)
				return line_number;
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("Distanz", list, 4);
			addjtmp("Zwischenzeit", list, 5);
			jdata["PNZWISCHENZEIT"].append(jtmp);
		}
		else if (list[0] == "PNREAKTION") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 6)
				return line_number;
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("Art", list, 4);
			addjtmp("Reaktionszeit", list, 5);
			jdata["PNREAKTION"].append(jtmp);
		}
		else if (list[0] == "STAFFEL") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 6)
				return line_number;
			addjtmp("Nummer der Mannschaft", list, 1);
			addjtmp("Veranstaltungs-ID der Mannschaft", list, 2);
			addjtmp("Wertungsklasse Typ", list, 3);
			addjtmp("Mindest-Jahrgang/Altersklasse", list, 4);
			addjtmp("Maximale Jahrgang/Altersklasse", list, 5);
			jdata["STAFFEL"].append(jtmp);
		}
		else if (list[0] == "STAFFELERGEBNIS") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 11)
				return line_number;
			addjtmp("Veranstaltungs-ID der Staffel", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("WertungsID", list, 4);
			addjtmp("Platz", list, 5);
			addjtmp("Endzeit", list, 6);
			addjtmp("Grund der Nichtwertung", list, 7);
			addjtmp("Startnummer des disqualifizierten Schwimmers innerhalb der Staffel", list, 8);
			addjtmp("Disqualifikationsbemerkung", list, 9);
			addjtmp("Erhöhtes nachträgliches Meldegeld", list, 10);
			jdata["STAFFELERGEBNIS"].append(jtmp);
		}
		else if (list[0] == "STZWISCHENZEIT") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 7)
				return line_number;
			addjtmp("Veranstaltungs-ID der Staffel", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("Startnummer des Schwimmers innerhalb der Staffel", list, 4);
			addjtmp("Distanz", list, 5);
			addjtmp("Zwischenzeit", list, 6);
			jdata["STZWISCHENZEIT"].append(jtmp);
		}
		else if (list[0] == "STABLOESE") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 7)
				return line_number;
			addjtmp("Veranstaltungs-ID der Staffel", list, 1);
			addjtmp("Wettkampf-Nr", list, 2);
			addjtmp("Wettkampfart", list, 3);
			addjtmp("Startnummer des Schwimmers innerhalb der Staffel", list, 4);
			addjtmp("Art", list, 5);
			addjtmp("Reaktionszeit", list, 6);
			jdata["STABLOESE"].append(jtmp);
		}
		else if (list[0] == "PNERGEBNIS") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 17)
				return line_number;
			addjtmp("Wettkampfnr.", list, 1);
			addjtmp("Wettkampfart", list, 2);
			addjtmp("WertungsID", list, 3);
			addjtmp("Platz", list, 4);
			addjtmp("Grund der Nichtwertung", list, 5);
			addjtmp("Name", list, 6);
			addjtmp("DSV-ID Schwimmer", list, 7);
			addjtmp("Veranstaltungs-ID des Schwimmers", list, 8);
			addjtmp("Geschlecht des Schwimmers", list, 9);
			addjtmp("Jahrgang", list, 10);
			addjtmp("Altersklasse", list, 11);
			addjtmp("Verein", list, 12);
			addjtmp("Vereinskennzahl", list, 13);
			addjtmp("Endzeit", list, 14);
			addjtmp("Disqualifikationsbemerkung", list, 15);
			addjtmp("Erhöhtes nachträgliches Meldegeld", list, 16);
			jdata["PNERGEBNIS"].append(jtmp);
		}
		else if (list[0] == "STERGEBNIS") // das kann öfters vorkommen
		{
			jtmp.clear();
			if (list.size() < 14)
				return line_number;
			addjtmp("Wettkampfnr.", list, 1);
			addjtmp("Wettkampfart", list, 2);
			addjtmp("WertungsID", list, 3);
			addjtmp("Platz", list, 4);
			addjtmp("Grund der Nichtwertung", list, 5);
			addjtmp("Nummer der Mannschaft", list, 6);
			addjtmp("Veranstaltungs-ID der Staffel", list, 7);
			addjtmp("Verein", list, 8);
			addjtmp("Vereinskennzahl", list, 9);
			addjtmp("Endzeit", list, 10);
			addjtmp("Startnummer des disqualifizierten Schwimmers innerhalb der Staffel", list, 11);
			addjtmp("Disqualifikationsbemerkung", list, 12);
			addjtmp("Erhöhtes nachträgliches Meldegeld", list, 13);
			jdata["STERGEBNIS"].append(jtmp);
		}
	} // while getline
	return 0;
}

int main(int ac, char **av)
{
	size_t return_value;
	std::ifstream in;
	std::ofstream out;

	ParseCommandLineArguments(ac, av);

	if (CommandLineArguments.empty())
	{
		Syntax();
		return 0;
	}

	if (GetCommandLineArgument("human", 'h'))
	{
		HumanReadAble = true;
	}

	if (GetCommandLineArgument("source", 's', SourceFile))
	{
		if (SourceFile.empty())
		{
			Syntax();
			return 0;
		}

		in.open(SourceFile);
		if (!in.is_open())
		{
			std::cout << "Kann Datei '" << SourceFile << "' nicht öffnen (lesen)." << std::endl;
			return 0;
		}
	}

	if (GetCommandLineArgument("output", 'o', OutputFile))
	{
		if (OutputFile.empty())
		{
			Syntax();
			return 0;
		}
		HasOutputFile = true;
		out.open(OutputFile);
		if (!out.is_open())
		{
			std::cout << "Kann Datei '" << OutputFile << "' nicht öffnen (schreiben)." << std::endl;
			return 0;
		}
	}

	// jetzt lesen wir endlich die Datei ein
	return_value = lese(in);

	if (return_value)
	{
		std::cout << "Fehler beim Parsen von " << SourceFile << " in Zeile " << return_value << std::endl;
	}

	std::cout << jdata << std::endl;

	Json::StyledWriter styledWriter;
	Json::FastWriter fastWriter;
	if (HasOutputFile)
	{
		if (HumanReadAble)
			out << styledWriter.write(jdata);
		else
			out << fastWriter.write(jdata);
	}
	else
	{
		if (HumanReadAble)
			std::cout << styledWriter.write(jdata) << std::endl;
		else
			std::cout << fastWriter.write(jdata) << std::endl;
	}

	return 1;
}
