 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/i18n/translator.h>
#include <smooth/i18n/language.h>

#include <smooth/files/directory.h>
#include <smooth/system/system.h>
#include <smooth/gui/application/application.h>
#include <smooth/misc/math.h>
#include <smooth/foreach.h>

#ifdef __WIN32__
#	include <windows.h>
#else
#	include <stdio.h>
#	include <stdlib.h>
#	include <unistd.h>
#endif

namespace smooth
{
	static String GetApplicationName()
	{
		String	 appName;

		for (Int i = 0; i < Object::GetNOfObjects(); i++)
		{
			if (Object::GetNthObject(i)->GetObjectType() != GUI::Application::classID) continue;

			GUI::Application	*application = (GUI::Application *) Object::GetNthObject(i);

			appName = application->GetText();
			appName.Replace(":", NIL).Replace("/", NIL);

			break;
		}

		return appName;
	}

	static Array<Directory> GetTranslationFolders(const String &appPrefix)
	{
		Array<Directory>	 dirs;

		/* Add application data folders.
		 */
		String	 appName = GetApplicationName();

		dirs.Add(S::System::System::GetApplicationDataDirectory().Append(appPrefix).Append(Directory::GetDirectoryDelimiter()).Append("lang"));

		if (appName != NIL && appName != appPrefix) dirs.Add(S::System::System::GetApplicationDataDirectory().Append(appName).Append(Directory::GetDirectoryDelimiter()).Append("lang"));

		/* Add application folder.
		 */
		dirs.Add(GUI::Application::GetApplicationDirectory().Append("lang"));

#ifndef __WIN32__
		/* Add resources folders.
		 */
		dirs.Add(S::System::System::GetResourcesDirectory().Append(appPrefix).Append(Directory::GetDirectoryDelimiter()).Append("lang"));

		if (appName != NIL && appName != appPrefix) dirs.Add(S::System::System::GetResourcesDirectory().Append(appName).Append(Directory::GetDirectoryDelimiter()).Append("lang"));
#endif

		return dirs;
	}
}

S::I18n::Translator	*S::I18n::Translator::defaultTranslator = NIL;

S::I18n::Translator::Translator(const String &iAppPrefix)
{
	appPrefix      = iAppPrefix.ToLower();
	activeLanguage = NIL;

	GetSupportedLanguages();

	ActivateLanguage("internal");
}

S::I18n::Translator::~Translator()
{
	foreach (Language *language, languages) delete language;

	languages.RemoveAll();
}

S::Int S::I18n::Translator::SetInternalLanguageInfo(const String &langName, const String &author, const String &url, Bool rightToLeft)
{
	Language	*iLang = NIL;

	for (Int n = 0; n < languages.Length(); n++)
	{
		if (languages.GetNth(n)->magic == "internal")
		{
			iLang = languages.GetNth(n);

			languages.Remove(languages.GetNthIndex(n));

			break;
		}
	}

	if (iLang != NIL)
	{
		iLang->SetName(langName);

		iLang->author	   = author;
		iLang->url	   = url;
		iLang->rightToLeft = rightToLeft;

		AddLanguage(iLang);
	}

	return Success();
}

S::String S::I18n::Translator::GetUserDefaultLanguageCode()
{
	String	 code;

#if defined __WIN32__
	LANGID	 langid = GetUserDefaultLangID();

	switch (PRIMARYLANGID(langid))
	{
		case LANG_ENGLISH:	code = "en";	if	(SUBLANGID(langid) == SUBLANG_ENGLISH_AUS)		  code = "en_AU";
							else if (SUBLANGID(langid) == SUBLANG_ENGLISH_CAN)		  code = "en_CA";
							else if (SUBLANGID(langid) == SUBLANG_ENGLISH_NZ)		  code = "en_NZ";
							else if (SUBLANGID(langid) == SUBLANG_ENGLISH_US)		  code = "en_US";
							else if (SUBLANGID(langid) == SUBLANG_ENGLISH_UK)		  code = "en_UK";
							break;
		case LANG_AFRIKAANS:	code = "af";	break;
		case LANG_ALBANIAN:	code = "sq";	break;
		case LANG_AMHARIC:	code = "am";	break;
		case LANG_ARABIC:	code = "ar";	if	(SUBLANGID(langid) == SUBLANG_ARABIC_ALGERIA)		  code = "ar_DZ";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_BAHRAIN)		  code = "ar_BH";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_EGYPT)		  code = "ar_EG";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_IRAQ)		  code = "ar_IQ";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_JORDAN)		  code = "ar_JO";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_KUWAIT)		  code = "ar_KW";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_LEBANON)		  code = "ar_LB";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_LIBYA)		  code = "ar_LY";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_MOROCCO)		  code = "ar_MO";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_OMAN)		  code = "ar_OM";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_QATAR)		  code = "ar_QA";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_SAUDI_ARABIA)	  code = "ar_SA";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_SYRIA)		  code = "ar_SY";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_TUNISIA)		  code = "ar_TN";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_UAE)		  code = "ar_AE";
							else if (SUBLANGID(langid) == SUBLANG_ARABIC_YEMEN)		  code = "ar_YE";
							break;
		case LANG_ARMENIAN:	code = "hy";	break;
		case LANG_ASSAMESE:	code = "as";	break;
		case LANG_AZERI:	code = "az";	if	(SUBLANGID(langid) == SUBLANG_AZERI_LATIN)		  code = "az_AZ@Latn";
							else if (SUBLANGID(langid) == SUBLANG_AZERI_CYRILLIC)		  code = "az_AZ@Cyrl";
							break;
		case LANG_BANGLA:	code = "bn";	if	(SUBLANGID(langid) == SUBLANG_BANGLA_BANGLADESH)	  code = "bn_BD";
							else if (SUBLANGID(langid) == SUBLANG_BANGLA_INDIA)		  code = "bn_IN";
							break;
		case LANG_BASQUE:	code = "eu";	break;
		case LANG_BELARUSIAN:	code = "be";	break;
		case LANG_BRETON:	code = "br";	break;
		case LANG_BULGARIAN:	code = "bg";	break;
		case LANG_CATALAN:	code = "ca";	break;
		case LANG_CHINESE:	code = "zh_TW";	if	(SUBLANGID(langid) == SUBLANG_CHINESE_SIMPLIFIED)	  code = "zh_CN";
							else if (SUBLANGID(langid) == SUBLANG_CHINESE_TRADITIONAL)	  code = "zh_TW";
							else if (SUBLANGID(langid) == SUBLANG_CHINESE_HONGKONG)		  code = "zh_HK";
							else if (SUBLANGID(langid) == SUBLANG_CHINESE_MACAU)		  code = "zh_MO";
							else if (SUBLANGID(langid) == SUBLANG_CHINESE_SINGAPORE)	  code = "zh_SG";
							break;
		case LANG_CORSICAN:	code = "co";	break;
		case LANG_CZECH:	code = "cs";	break;
		case LANG_DANISH:	code = "da";	break;
		case LANG_DARI:		code = "prs";	break;
		case LANG_DIVEHI:	code = "dv";	break;
		case LANG_DUTCH:	code = "nl";	if	(SUBLANGID(langid) == SUBLANG_DUTCH)			  code = "nl_NL";
							else if (SUBLANGID(langid) == SUBLANG_DUTCH_BELGIAN)		  code = "nl_BE";
							break;
		case LANG_ESTONIAN:	code = "et";	break;
		case LANG_FILIPINO:	code = "fil";	break;
		case LANG_FINNISH:	code = "fi";	break;
		case LANG_FRENCH:	code = "fr";	if	(SUBLANGID(langid) == SUBLANG_FRENCH)			  code = "fr_FR";
							else if (SUBLANGID(langid) == SUBLANG_FRENCH_BELGIAN)		  code = "fr_BE";
							else if (SUBLANGID(langid) == SUBLANG_FRENCH_CANADIAN)		  code = "fr_CA";
							else if (SUBLANGID(langid) == SUBLANG_FRENCH_LUXEMBOURG)	  code = "fr_LU";
							else if (SUBLANGID(langid) == SUBLANG_FRENCH_MONACO)		  code = "fr_MC";
							else if (SUBLANGID(langid) == SUBLANG_FRENCH_SWISS)		  code = "fr_CH";
							break;
		case LANG_GALICIAN:	code = "gl";	break;
		case LANG_GEORGIAN:	code = "ka";	break;
		case LANG_GERMAN:	code = "de";	break;
		case LANG_GREEK:	code = "el";	break;
		case LANG_GUJARATI:	code = "gu";	break;
		case LANG_HAUSA:	code = "ha";	break;
		case LANG_HEBREW:	code = "he";	break;
		case LANG_HINDI:	code = "hi";	break;
		case LANG_HUNGARIAN:	code = "hu";	break;
		case LANG_ICELANDIC:	code = "is";	break;
		case LANG_IGBO:		code = "ig";	break;
		case LANG_INDONESIAN:	code = "id";	break;
		case LANG_IRISH:	code = "ga";	break;
		case LANG_ITALIAN:	code = "it";	break;
		case LANG_JAPANESE:	code = "ja";	break;
		case LANG_KANNADA:	code = "kn";	break;
		case LANG_KAZAK:	code = "kk";	break;
		case LANG_KHMER:	code = "kh";	break;
		case LANG_KINYARWANDA:	code = "rw";	break;
		case LANG_KOREAN:	code = "ko";	break;
		case LANG_KYRGYZ:	code = "ky";	break;
		case LANG_LAO:		code = "lo";	break;
		case LANG_LATVIAN:	code = "lv";	break;
		case LANG_LITHUANIAN:	code = "lt";	break;
		case LANG_LUXEMBOURGISH:code = "lb";	break;
		case LANG_MACEDONIAN:	code = "mk";	break;
		case LANG_MALAY:	code = "ms";	break;
		case LANG_MALAYALAM:	code = "ml";	break;
		case LANG_MALTESE:	code = "mt";	break;
		case LANG_MAORI:	code = "mi";	break;
		case LANG_MARATHI:	code = "mr";	break;
		case LANG_MONGOLIAN:	code = "mn";	break;
		case LANG_NEPALI:	code = "ni";	break;
		case LANG_NORWEGIAN:	code = "no";	break;
		case LANG_OCCITAN:	code = "oc";	break;
		case LANG_ORIYA:	code = "or";	break;
		case LANG_PASHTO:	code = "ps";	break;
		case LANG_PERSIAN:	code = "fa";	break;
		case LANG_POLISH:	code = "pl";	break;
		case LANG_PORTUGUESE:	code = "pt";	if	(SUBLANGID(langid) == SUBLANG_PORTUGUESE)		  code = "pt_PT";
							else if (SUBLANGID(langid) == SUBLANG_PORTUGUESE_BRAZILIAN)	  code = "pt_BR";
							break;
		case LANG_PULAR:	code = "ff";	break;
		case LANG_PUNJABI:	code = "pa";	if	(SUBLANGID(langid) == SUBLANG_PUNJABI_INDIA)		  code = "pa_IN";
							else if (SUBLANGID(langid) == SUBLANG_PUNJABI_PAKISTAN)		  code = "pa_PK";
							break;
		case LANG_QUECHUA:	code = "quz";	break;
		case LANG_ROMANIAN:	code = "ro";	break;
		case LANG_RUSSIAN:	code = "ru";	break;
		case LANG_SERBIAN:	code = "sr";	if	(SUBLANGID(langid) == SUBLANG_CROATIAN_CROATIA)		  code = "hr_HR";
							else if (SUBLANGID(langid) == SUBLANG_SERBIAN_LATIN)		  code = "sr_RS@Latn";
							else if (SUBLANGID(langid) == SUBLANG_SERBIAN_CYRILLIC)		  code = "sr_RS@Cyrl";
							break;
		case LANG_SINHALESE:	code = "si";	break;
		case LANG_SLOVAK:	code = "sk";	break;
		case LANG_SLOVENIAN:	code = "sl";	break;
		case LANG_SOTHO:	code = "sa";	break;
		case LANG_SPANISH:	code = "es";	if	(SUBLANGID(langid) == SUBLANG_SPANISH)			  code = "es_ES";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_ARGENTINA)	  code = "es_AR";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_BOLIVIA)		  code = "es_BO";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_CHILE)		  code = "es_CL";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_COLOMBIA)		  code = "es_CO";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_COSTA_RICA)	  code = "es_CR";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_DOMINICAN_REPUBLIC) code = "es_DO";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_ECUADOR)		  code = "es_EC";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_EL_SALVADOR)	  code = "es_SV";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_GUATEMALA)	  code = "es_GT";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_HONDURAS)		  code = "es_HN";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_MEXICAN)		  code = "es_MX";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_NICARAGUA)	  code = "es_NI";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_PANAMA)		  code = "es_PA";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_PARAGUAY)		  code = "es_PY";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_PERU)		  code = "es_PE";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_PUERTO_RICO)	  code = "es_PR";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_URUGUAY)		  code = "es_UY";
							else if (SUBLANGID(langid) == SUBLANG_SPANISH_VENEZUELA)	  code = "es_VE";
							break;
		case LANG_SWAHILI:	code = "sw";	break;
		case LANG_SWEDISH:	code = "sv";	break;
		case LANG_TAJIK:	code = "tg";	break;
		case LANG_TAMIL:	code = "ta";	if	(SUBLANGID(langid) == SUBLANG_TAMIL_INDIA)		  code = "ta_IN";
							else if (SUBLANGID(langid) == SUBLANG_TAMIL_SRI_LANKA)		  code = "ta_LK";
							break;
		case LANG_TELUGU:	code = "te";	break;
		case LANG_THAI:		code = "th";	break;
		case LANG_TIBETAN:	code = "bo";	break;
		case LANG_TIGRINYA:	code = "ti";	break;
		case LANG_TSWANA:	code = "tn";	break;
		case LANG_TURKISH:	code = "tr";	break;
		case LANG_TURKMEN:	code = "tk";	break;
		case LANG_UIGHUR:	code = "ug";	break;
		case LANG_UKRAINIAN:	code = "uk";	break;
		case LANG_URDU:		code = "ur";	if	(SUBLANGID(langid) == SUBLANG_URDU_INDIA)		  code = "ur_IN";
							else if (SUBLANGID(langid) == SUBLANG_URDU_PAKISTAN)		  code = "ur_PK";
							break;
		case LANG_UZBEK:	code = "uz";	if	(SUBLANGID(langid) == SUBLANG_UZBEK_LATIN)		  code = "uz_UZ@Latn";
							else if (SUBLANGID(langid) == SUBLANG_UZBEK_CYRILLIC)		  code = "uz_UZ@Cyrl";
							break;
		case LANG_VIETNAMESE:	code = "vi";	break;
		case LANG_WELSH:	code = "cy";	break;
		case LANG_WOLOF:	code = "wo";	break;
		case LANG_XHOSA:	code = "xh";	break;
		case LANG_YI:		code = "ii";	break;
		case LANG_YORUBA:	code = "yo";	break;
		case LANG_ZULU:		code = "zu";	break;
	}
#elif defined __APPLE__
	FILE		*pstdin = popen("defaults read -g AppleLocale", "r");
	Buffer<char>	 buffer(256);

	buffer.Zero();

	fscanf(pstdin, String("%[^\n]").Append(String::FromInt(buffer.Size() - 1)), (char *) buffer);

	pclose(pstdin);

	code = (char *) buffer;
#elif defined __HAIKU__
	FILE		*pstdin = popen("locale -l", "r");
	Buffer<char>	 buffer(256);

	buffer.Zero();

	fscanf(pstdin, String("%[^\n]").Append(String::FromInt(buffer.Size() - 1)), (char *) buffer);

	pclose(pstdin);

	code = (char *) buffer;
#else
	code = getenv("LANG");

	if (code == NIL) code = getenv("LC_MESSAGES");
#endif

	if (code.Contains(".")) code = code.Head(code.Find(".")).Append(code.Contains("@") ? code.Tail(code.Length() - code.Find("@")) : "");

	code.Replace("@latin", "@Latn");
	code.Replace("@cyrillic", "@Cyrl");

	return code;
}

S::Int S::I18n::Translator::SelectUserDefaultLanguage()
{
	String	 code = GetUserDefaultLanguageCode();

	/* Try the language code, possibly with appended country and script code.
	 */
	if (ActivateLanguage(String(appPrefix).Append("_").Append(code).Append(".xml")) == Success()) return Success();

	/* If failed, try without the script code if applicable.
	 */
	if (code.Contains("@"))
	{
		if (ActivateLanguage(String(appPrefix).Append("_").Append(code.Head(code.Find("@"))).Append(".xml")) == Success()) return Success();
	}

	/* If failed, try without the country code if applicable.
	 */
	if (code.Contains("_") && code.Contains("@"))
	{
		if (ActivateLanguage(String(appPrefix).Append("_").Append(code.Head(code.Find("_"))).Append(code.Contains("@") ? code.Tail(code.Length() - code.Find("@")) : "").Append(".xml")) == Success()) return Success();
	}

	/* If failed, try the base language code if applicable.
	 */
	if (code.Contains("_"))
	{
		if (ActivateLanguage(String(appPrefix).Append("_").Append(code.Head(code.Find("_"))).Append(".xml")) == Success()) return Success();
	}

	/* Fall back to alternative language.
	 */
	if	(code.StartsWith("fil")	  && ActivateLanguage(String(appPrefix).Append("_tl.xml"))    == Success()) return Success(); // Fall back to Tagalog for Filipino
	else if	(code.StartsWith("prs")	  && ActivateLanguage(String(appPrefix).Append("_fa.xml"))    == Success()) return Success(); // Fall back to Persian for Dari
	else if (code.StartsWith("zh_MO") && ActivateLanguage(String(appPrefix).Append("_zh_HK.xml")) == Success()) return Success(); // Fall back to Cantonese (Hong Kong) for Cantonese (Macau)

	/* Fall back to internal language.
	 */
	ActivateLanguage("internal");

	return Success();
}

S::Int S::I18n::Translator::GetSupportedLanguages()
{
	/* Add default translation.
	 */
	{
		Language	*language = new Language();

		language->SetName("Default language");

		language->encoding    = "UTF-8";
		language->magic	      = "internal";
		language->author      = "unknown";
		language->url	      = "none";
		language->rightToLeft = False;

		languages.Add(language);
	}

	/* Check translation folders.
	 */
	const Array<Directory>	&dirs = GetTranslationFolders(appPrefix);

	foreach (const Directory &dir, dirs)
	{
		if (!dir.Exists() || dir.GetFilesByPattern(String(appPrefix).Append("_*.xml")).Length() == 0) continue;

		/* Load translation files.
		 */
		const Array<File>	&files = dir.GetFilesByPattern(String(appPrefix).Append("_*.xml"));

		foreach (const File &file, files)
		{
			XML::Document	 doc;

			if (doc.LoadFile(file) == Success())
			{
				Language	*language = new Language();

				language->magic = file.GetFileName();

				if (LoadDescription(doc, language) == Success()) AddLanguage(language);
			}
		}
	}

	return Success();
}

S::Bool S::I18n::Translator::SetContext(const String &nContext)
{
	return activeLanguage->SetContext(nContext);
}

const S::String &S::I18n::Translator::GetContext() const
{
	return activeLanguage->GetContext();
}

S::Int S::I18n::Translator::GetNOfLanguages() const
{
	return languages.Length();
}

const S::String &S::I18n::Translator::GetNthLanguageName(Int index) const
{
	return languages.GetNth(index)->GetName();
}

const S::String &S::I18n::Translator::GetNthLanguageID(Int index) const
{
	return languages.GetNth(index)->magic;
}

const S::String &S::I18n::Translator::GetNthLanguageAuthor(Int index) const
{
	return languages.GetNth(index)->author;
}

const S::String &S::I18n::Translator::GetNthLanguageEncoding(Int index) const
{
	return languages.GetNth(index)->encoding;
}

const S::String &S::I18n::Translator::GetNthLanguageURL(Int index) const
{
	return languages.GetNth(index)->url;
}

S::Bool S::I18n::Translator::IsNthLanguageRightToLeft(Int index) const
{
	return languages.GetNth(index)->rightToLeft;
}

S::Bool S::I18n::Translator::IsNthLanguageIncomplete(Int index) const
{
	return languages.GetNth(index)->incomplete;
}

const S::String &S::I18n::Translator::GetActiveLanguageName() const
{
	return activeLanguage->GetName();
}

const S::String &S::I18n::Translator::GetActiveLanguageID() const
{
	return activeLanguage->magic;
}

const S::String &S::I18n::Translator::GetActiveLanguageAuthor() const
{
	return activeLanguage->author;
}

const S::String &S::I18n::Translator::GetActiveLanguageEncoding() const
{
	return activeLanguage->encoding;
}

const S::String &S::I18n::Translator::GetActiveLanguageURL() const
{
	return activeLanguage->url;
}

S::Bool S::I18n::Translator::IsActiveLanguageRightToLeft() const
{
	return activeLanguage->rightToLeft;
}

S::Bool S::I18n::Translator::IsActiveLanguageIncomplete() const
{
	return activeLanguage->incomplete;
}

S::Int S::I18n::Translator::ActivateLanguage(const String &magic)
{
	foreach (Language *language, languages)
	{
		if (language->magic != magic) continue;

		activeLanguage = language;

		/* Try to set application language for internal library strings.
		 */
		if (magic != "internal")
		{
			String	 code = magic.SubString(magic.Find("_") + 1, magic.Find(".xml") - magic.Find("_") - 1);
			String	 id   = String("smooth_").Append(code).Append(".xml");

			if (defaultTranslator->ActivateLanguage(id) != Success())
			{
				/* If failed, try the base language code if applicable.
				 */
				if (code.Contains("_"))
				{
					id = String("smooth_").Append(code.Head(code.Find("_"))).Append(".xml");

					defaultTranslator->ActivateLanguage(id);
				}
			}

			/* Activate user default language if setting the application language failed.
			 */
			if (defaultTranslator->GetActiveLanguageID() != id) defaultTranslator->SelectUserDefaultLanguage();
		}

		/* Activate internal language if application language is set to internal.
		 */
		if (magic == "internal" && defaultTranslator != NIL) defaultTranslator->ActivateLanguage(magic);

		/* Load actual language data.
		 */
		if (magic != "internal" && activeLanguage->strings.Length() == 0 && activeLanguage->sections.Length() == 0)
		{
			/* Check translation folders.
			 */
			const Array<Directory>	&dirs = GetTranslationFolders(appPrefix);

			foreach (const Directory &dir, dirs)
			{
				/* Load translation file.
				 */
				String		 file = String(dir).Append(Directory::GetDirectoryDelimiter()).Append(magic);
				XML::Document	 doc;

				if (doc.LoadFile(file) == Success() && LoadData(doc, activeLanguage) == Success()) break;
			}
		}

		return Success();
	}

	return Error();
}

const S::String &S::I18n::Translator::TranslateString(const String &string)
{
	const String	&translation = activeLanguage->GetString(string);

	if (translation == NIL)	return string;
	else			return translation;
}

const S::String &S::I18n::Translator::TranslateString(const String &string, const String &context)
{
	String	 prevContext = activeLanguage->GetContext();

	activeLanguage->SetContext(context);

	const String	&translation = activeLanguage->GetString(string);

	activeLanguage->SetContext(prevContext);

	if (translation == NIL)	return string;
	else			return translation;
}

S::Int S::I18n::Translator::AddLanguage(Language *language)
{
	/* Check arguments and return an error if they are not sane.
	 */
	if (language == NIL) return Error();

	const String	&langName = language->GetName();

	for (Int i = 0; i < languages.Length(); i++)
	{
		const String	&compName = languages.GetNth(i)->GetName();

		for (Int j = 0; j < Math::Max(langName.Length(), compName.Length()); j++)
		{
			if (langName == compName) return Error();

			if ((langName[j] < compName[j] && !(langName[j] == '(' && compName[j] == '/')) || (langName[j] == '/' && compName[j] == '('))
			{
				languages.InsertAtPos(i, language);

				return Success();
			}

			if ((langName[j] > compName[j] && !(langName[j] == '/' && compName[j] == '(')) || (langName[j] == '(' && compName[j] == '/')) break;
		}
	}

	languages.Add(language);

	return Success();
}

S::Int S::I18n::Translator::LoadDescription(const XML::Document &doc, Language *language)
{
	/* Check arguments and return an error if they are not sane.
	 */
	if (doc.GetRootNode() == NIL || language == NIL) return Error();

	XML::Node	*root	= doc.GetRootNode();
	Error		 result = language->ParseHeader(root);

	if (result != Success()) return Error();

	return Success();
}

S::Int S::I18n::Translator::LoadData(const XML::Document &doc, Language *language)
{
	/* Check arguments and return an error if they are not sane.
	 */
	if (doc.GetRootNode() == NIL || language == NIL) return Error();

	XML::Node	*root	= doc.GetRootNode();
	Error		 result = language->Parse(root);

	if (result != Success()) return Error();

	return Success();
}
