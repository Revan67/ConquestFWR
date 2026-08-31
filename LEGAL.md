# Legal and Compliance Policy

This document records the project's good-faith compliance procedures. It is not legal advice, a
commercial license, or an amendment or addendum to the controlling license. It grants no rights.
If this document conflicts with the license, the license controls.

## Controlling license

The Program is provided under the unmodified [Public License for Conquest: Frontier Wars Program
and Source Code](Conquest%20Source%20License.txt), copyright Fever Pitch Studios, December 2013.
The license defines the Program broadly to include the source and accompanying assets, files, data,
executables, object code, machine-readable derivatives, libraries, and utilities within or relating
to *Conquest: Frontier Wars*.

This repository is a modified **Program Derivative**. The entire Program Derivative, as a whole,
is released solely under the exact same terms as the controlling license. Each recipient
automatically receives that identical license. Nothing in this repository may modify its terms or
attach additional license terms to the Program.

The license is revocable and permits **non-commercial use only**. No money, property, services, or
other consideration may be given or received in exchange for use or conveyance of the Program or a
Program Derivative, or for warranty or support. The prohibition expressly includes payments,
royalties, expenses, donations, gratuities, usage fees, and transmission fees.

Fever Pitch Studios supplies the Program **as is**, without warranty or a support obligation, and
limits its liability as stated in sections 7 and 8 of the controlling license. Those notices must
remain intact. The license selects Texas law.

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
a source archive or compiled build. Contributors must add a dated, attributed entry for material
changes before those changes are conveyed.

The public author identity for project modifications is **Revan67**. Do not place a contributor's
private name, address, account path, crash dump, or other personal information into a public notice.

## Retail material and third-party components

Development-source files that accompanied the official source release are treated as Program
material under the controlling license. A retail installation is operational input used for testing;
it is not a source of files for this repository or public release packages.

Do not commit or redistribute:

- retail executables, DLLs, installers, disc images, archives, or patches obtained separately;
- video, audio, textures, maps, missions, manuals, or other media extracted from a retail copy;
- proprietary SDK redistributables or third-party binaries without documented redistribution rights;
- serial numbers, keys, credentials, copy-protection data, or access-control secrets; or
- substantial disassembly, decompiler output, binary dumps, or copyrighted code copied from retail.

The repository contains historical binaries and tools from the development-source tree. Their
presence must not be taken as proof that every third-party component may be redistributed outside
that source release. In particular, the separately added `binkw32.dll` and
`Conquest_Frontier_Wars_Manual.pdf` need documented provenance and redistribution permission before
they are included in any new public release package. Until that review is complete, release tooling
must exclude them and maintainers should not represent them as covered by the source license.

## Reverse engineering for interoperability

The project's compatibility research has a narrow purpose: make an independently rebuilt Program
interoperate with the data, campaign modules, and observable behavior of a lawfully obtained retail
copy on modern Windows.

In the United States, [17 U.S.C. section 1201(f)](https://uscode.house.gov/view.xhtml?edition=prelim&num=0&req=granuleid%3AUSC-prelim-title17-section1201)
provides a conditional interoperability exception. Among other requirements, the person must have
lawfully obtained the right to use a copy; the work must be limited to identifying and analyzing
elements necessary for interoperability of an independently created program; those elements must
not previously have been readily available; and the acts must not otherwise infringe copyright.
Information or means may be shared only as the statute permits and solely to enable interoperability.
The statute defines interoperability as computer programs exchanging information and mutually using
the information exchanged.

That exception is not a blanket authorization to reverse engineer, copy, publish decompiled code,
or bypass technological protection measures. Laws and contract terms vary by jurisdiction. A person
performing analysis is responsible for having a lawful copy and for complying with applicable law
and enforceable agreements.

Permitted project evidence should normally be limited to independently recorded functional facts,
such as:

- exported or decorated symbol names needed for module linkage;
- structure sizes, field offsets, enum values, and file-format behavior needed to parse compatible
  data;
- API call sequences, rendering conventions, and black-box input/output behavior; and
- original tests, tables, documentation, and clean-room code expressing those facts.

Researchers must not add copied retail implementation text or substantial expression. Keep raw
retail binaries, disassembly databases, decompiler projects, traces, and dumps outside the repository.
Published notes should contain only the minimum functional facts and independently written analysis
needed to reproduce compatibility.

The project does not authorize or require circumvention of copy protection. If work would require
defeating an effective access control, distributing circumvention technology, using material whose
lawful provenance is uncertain, or going beyond interoperability, stop and obtain specific permission
or qualified legal advice before proceeding.

For general context, the U.S. Copyright Office explains that section 1201 generally prohibits
circumvention while containing statutory and periodically adopted exemptions:
[Section 1201 study](https://www.copyright.gov/policy/1201/) and
[2024 rulemaking](https://www.copyright.gov/1201/2024/).

## Contribution and release checklist

Before accepting or releasing a change, confirm that:

- the work is non-commercial and no prohibited consideration is involved;
- the contributor has authority to provide the change under the controlling license;
- `MODIFICATIONS.md` identifies the relevant author, date, and nature of the modification;
- all code and prose are independently written or have documented compatible provenance;
- no separately obtained retail content or raw reverse-engineering output is included;
- every dependency and binary has a recorded source and redistribution basis;
- the complete, unmodified controlling license and all required notices accompany the copy; and
- the package does not add terms or obligations that conflict with the controlling license.

If any item cannot be confirmed, do not distribute the affected material until it is resolved.
