/**
 * @file ClipSavePolicy.h
 * @brief Declares DittoCore::ClipSaveSettings and DittoCore::ClipSavePolicy.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>

namespace DittoCore
{
	/**
	 * @brief The options that decide how a copied clip is saved, injected into the clip instead of
	 *        read from the global options.
	 */
	struct ClipSaveSettings
	{
		/// Whether a clip whose CRC is already saved is saved again.
		bool allowDuplicates{};
		/// With allowDuplicates: whether the same clip copied twice in a row is saved twice.
		bool allowBackToBackDuplicates{};
		/// Whether the CRC leaves out the parts of a clip that change on every copy.
		bool adjustForCrc{};
		/// Whether a copy that the source marks as not for clipboard history is skipped.
		bool enforceIgnoreFormats{};
		/// The largest clipboard format saved, in bytes; 0 for no limit.
		std::int64_t maxClipSizeInBytes{};
		/// The longest description kept, in characters.
		std::size_t descriptionLength{};
		/// The largest file whose contents are read into a "file contents" clip, in bytes.
		std::uint64_t maxFileContentsSize{};
		/// Programs (lower-case exe names) whose bitmap is not saved when they also copy text.
		std::set<std::wstring> ignoreDibFromApps{};
		/// The sound file played after a clip is saved; empty for none.
		std::wstring playSoundOnCopy{};
	};

	/// Which earlier clip a new copy is checked against before it is saved.
	enum class DuplicateCheck
	{
		/// Save the copy as a new clip.
		None,
		/// The copy repeats the last saved clip: reuse that clip.
		LastAdded,
		/// Reuse any saved clip with the same CRC (looked up in the database).
		AnyByCrc
	};

	/**
	 * @brief The save decisions taken from ClipSaveSettings.
	 */
	class ClipSavePolicy
	{
	public:
		/**
		 * @brief Takes the settings.
		 * @param settings The options to apply.
		 */
		explicit ClipSavePolicy(ClipSaveSettings settings);

		/**
		 * @brief Which earlier clip a new copy is checked against.
		 * @param crc The new copy's CRC.
		 * @param lastAddedCrc The CRC of the clip saved last.
		 * @return AnyByCrc without duplicates; LastAdded for a back-to-back repeat when those are
		 *         not allowed; None otherwise.
		 */
		DuplicateCheck DuplicateCheckFor(std::uint32_t crc, std::uint32_t lastAddedCrc) const;

		/**
		 * @brief Whether a clipboard format is too large to save.
		 * @param sizeInBytes The format's size.
		 * @return True when a limit is set and the size is above it.
		 */
		bool TooLarge(std::uint64_t sizeInBytes) const;

		/**
		 * @brief Whether a program's bitmap is left out of a copy that also has text.
		 * @param lowerCaseApp The copying program's exe name in lower case.
		 * @return True when the program is in ignoreDibFromApps.
		 */
		bool IgnoresDibFrom(const std::wstring& lowerCaseApp) const;

		/**
		 * @brief The settings the policy applies.
		 * @return The settings.
		 */
		const ClipSaveSettings& Settings() const;

	private:
		/// The options.
		ClipSaveSettings m_settings;
	};
}
