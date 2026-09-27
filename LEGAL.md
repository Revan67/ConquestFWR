# Legal and Compliance Policy

This document records the project's good-faith compliance procedures. It is not legal advice, a
commercial license, or an amendment or addendum to the controlling license. It grants no rights.
If this document conflicts with the license, the license controls.

## Controlling license

The controlling text is the repository's [Public License for Conquest: Frontier Wars Program and
Source Code](Conquest%20Source%20License.txt), copyright © December 2013, Fever Pitch Studios.
Read that file for its terms, including the non-commercial restriction, warranty disclaimer, and
limitation of liability. This policy does not restate or modify those terms.

ConquestFWR is a modified **Program Derivative** conveyed under that same license. Preserve the
license and the prominent modification notice when distributing it. The repository copy of the
license remains subject to a completeness concern: section 6 ends mid-sentence in both the local
source-package copy and an independent source mirror. Do not invent an ending or claim the text is
complete without stronger original evidence or permission from the rights holder.

## Required notice for every distribution

Before publishing, packaging, mirroring, or otherwise conveying any source or compiled build:

1. Include `Conquest Source License.txt` in full, intact, and conspicuous form.
2. Include `MODIFICATIONS.md` and keep the relevant modification dates and authors current.
3. State prominently that the material is modified and that the entire Program Derivative and all
   recipients are subject to the exact same controlling license.
4. Preserve the copyright, no-warranty, and limitation-of-liability notices.
5. Do not request or accept compensation, donations, reimbursement, or other consideration for the
   Program, the derivative, distribution, warranty, or support.
6. Do not combine or distribute the Program in a way that imposes conflicting license obligations.
   If all obligations cannot be satisfied simultaneously, do not use, modify, or convey the affected
   material.
7. Review every third-party component separately. Do not assume that the Fever Pitch Studios
   license grants rights that a third party owns.

`Build-Conquest.ps1` copies the license, this policy, and the modification record into deployment
directories. That convenience does not replace the distributor's obligation to review the complete
package.

## Modification notices

[MODIFICATIONS.md](MODIFICATIONS.md) is the repository's prominent modification record. Git history
provides the detailed file-level record, but Git history alone must not be relied on when distributing
a source archive or compiled build. Check and update the dated, attributed ledger before each commit
intended for the public repository, as well as before distribution.

The public author identity for project modifications is **Revan67**. Do not place a contributor's
private name, address, account path, crash dump, or other personal information into a public notice.

## Retail material and third-party components

Development-source files that accompanied the official source release are treated as Program
material under the controlling license, subject to any separately owned rights. A retail installation
is operational input used for testing; it is not a source of files for this repository or public release
packages. Original Conquest documentation is not Bink middleware, but its origin as game material
does not override an express notice from another rights holder.

Do not commit or redistribute:

- retail executables, DLLs, installers, disc images, archives, or patches obtained separately;
- video, audio, textures, maps, missions, manuals, or other media extracted from a retail copy;
- proprietary SDK redistributables or third-party binaries without documented redistribution rights;
- serial numbers, keys, credentials, copy-protection data, or access-control secrets; or
- substantial disassembly, decompiler output, binary dumps, or copyrighted code copied from retail.

The repository contains historical binaries and tools from the development-source tree. Their
presence must not be taken as proof that every third-party component may be redistributed outside
that source release. The two `Code/Libs/ImplicitDLL/` Bink DLLs were in the initial source import;
the repository-root Bink DLL was added later and matches a local retail installation. All three carry
RAD Game Tools copyright notices, and no accompanying redistribution grant was found. They are
excluded from current Git tracking while local copies remain available for lawful local use.

`Conquest_Frontier_Wars_Manual.pdf` is a Conquest game field manual, not Bink documentation. It was
added to Git in May 2026, not with the initial source import, and differs from the manual in the local
retail installation. Its copyright page names Digital Anvil and Microsoft and requires Microsoft's
express written permission for reproduction. It is excluded from current Git tracking pending a
documented right to convey this edition. These index removals do not erase earlier Git history or
recall prior distributions.

## Reverse engineering for interoperability

The project's compatibility research has a narrow purpose: make an independently rebuilt Program
interoperate with the data, campaign modules, and observable behavior of a lawfully obtained retail
copy on modern Windows.

In the United States, [17 U.S.C. section 1201(f)](https://uscode.house.gov/view.xhtml?edition=prelim&num=0&req=granuleid%3AUSC-prelim-title17-section1201)
addresses certain interoperability research. Whether it applies to a particular act depends on its
facts and all statutory conditions; this project policy is not a determination of eligibility. It is
not automatic clearance to copy or publish decompiled code or to bypass an access control. Laws and
contract terms vary by jurisdiction. Researchers must use lawfully obtained material and seek
specific permission or qualified advice where the legal basis is uncertain.

Permitted project evidence should normally be limited to independently recorded functional facts,
such as:

- exported or decorated symbol names needed for module linkage;
- structure sizes, field offsets, enum values, and file-format behavior needed to parse compatible
  data;
- API call sequences, rendering conventions, and black-box input/output behavior; and
- original tests, tables, documentation, and clean-room code expressing those facts.

Researchers must not add copied retail implementation text or substantial expression. Keep raw
retail binaries, disassembly databases, decompiler projects, traces, and dumps out of the publicly
tracked repository. They may remain in local/private evidence storage. Published notes should contain
only the minimum functional facts and independently written analysis needed for compatibility.

The project does not authorize or require circumvention of copy protection. If work would require
defeating an effective access control, distributing circumvention technology, using material whose
lawful provenance is uncertain, or going beyond interoperability, stop and obtain specific permission
or qualified legal advice before proceeding.

For general context, the U.S. Copyright Office explains that section 1201 generally prohibits
circumvention while containing statutory and periodically adopted exemptions:
[Section 1201 study](https://www.copyright.gov/policy/1201/) and
[2024 rulemaking](https://www.copyright.gov/1201/2024/).

## Pre-commit and release checklist

Before staging or committing material intended for the public repository, and again before releasing
a copy, confirm that:

- the work is non-commercial and no prohibited consideration is involved;
- the contributor has authority to provide the change under the controlling license;
- `MODIFICATIONS.md` identifies the relevant author, date, and nature of the modification;
- all code and prose are independently written or have documented compatible provenance;
- no separately obtained retail content or raw reverse-engineering output is included;
- every dependency and binary has a recorded source and redistribution basis;
- the complete, unmodified controlling license and all required notices accompany the copy; and
- the package does not add terms or obligations that conflict with the controlling license.

Do not stage unreviewed third-party material, generated binaries unrelated to the change, or raw
retail evidence. Check that the staged set contains only the intended files and no gameplay source
when the task is documentation-only. If any item cannot be confirmed, do not commit or distribute the
affected material until it is resolved. The source-package license truncation remains an explicit
public-conveyance question, not something this checklist silently clears.
