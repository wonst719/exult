#ifndef INCL_WINDRAG
#define INCL_WINDRAG

#if defined(_WIN32) && defined(USE_EXULTSTUDIO)

#ifdef __GNUC__
// COM sucks.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#endif

#include <atomic>
#include <vector>

#include "u7drag.h"
#include <ole2.h>
#include "utils.h"

// A useful structure for Winstudioobj
class windragdata {
	std::vector<unsigned char> data;
	sint32 id;
public:

	inline unsigned char *get_data() {
		return data.data();
	}

	inline int get_id() const {
		return id;
	}

	inline size_t get_size() const {
		return data.size();
	}

	// Default constructor
	inline windragdata() = default;

	// Read from buffer
	explicit inline windragdata(const unsigned char *buf) {
		id = Read4(buf);
		size_t size = Read4(buf);
		data.assign(buf, buf + size);
	}

	inline windragdata(sint32 i, uint32 s, const unsigned char *d)
		: data(d, d + s), id(i) {
	}

	inline void serialize(unsigned char *buf) {
		Write4(buf, id);
		Write4(buf, data.size());
		std::memcpy(buf, data.data(), data.size());
	}

	inline void assign(sint32 i, uint32 s, const unsigned char *d) {
		id = i;
		data.assign(d, d + s);
	}
};

/*
 * The 'IDropTarget' implementation
 */
class FAR Windnd final : public IDropTarget {
private:
	HWND gamewin;

	std::atomic<DWORD> m_cRef;

	void *udata;

	Move_shape_handler_fun move_shape_handler;
	Move_combo_handler_fun move_combo_handler;
	Drop_shape_handler_fun shape_handler;
	Drop_chunk_handler_fun chunk_handler;
	Drop_npc_handler_fun npc_handler;
	Drop_shape_handler_fun face_handler;
	Drop_combo_handler_fun combo_handler;

	// Used for when dragging the mouse over the exult window
	int drag_id;
	int prevx, prevy;
	union {
		struct {
			int file, shape, frame;
		} shape;
		struct  {
			int chunknum;
		} chunk;
		struct {
			int xtiles, ytiles;
			int right, below, cnt;
			U7_combo_data *combo;
		} combo;
		struct  {
			int npcnum;
		} npc;
	} data;
	~Windnd() = default;

public:
	Windnd(HWND hgwnd, Move_shape_handler_fun, Move_combo_handler_fun,
	       Drop_shape_handler_fun, Drop_chunk_handler_fun,
	       Drop_npc_handler_fun npcfun, Drop_combo_handler_fun);
	Windnd(HWND hgwnd, Drop_shape_handler_fun shapefun,
	       Drop_chunk_handler_fun cfun, Drop_shape_handler_fun ffun, void *d);

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

	STDMETHOD(DragEnter)(IDataObject *pDataObject,
	                     DWORD grfKeyState,
	                     POINTL pt,
	                     DWORD *pdwEffect) override;
	STDMETHOD(DragOver)(DWORD grfKeyState,
	                    POINTL pt,
	                    DWORD *pdwEffect) override;
	STDMETHOD(DragLeave)() override;
	STDMETHOD(Drop)(IDataObject *pDataObject,
	                DWORD grfKeyState,
	                POINTL pt,
	                DWORD *pdwEffect) override;

	bool is_valid(IDataObject *pDataObject);

	static void CreateStudioDropDest(Windnd  *&windnd, HWND &hWnd,
	                                 Drop_shape_handler_fun shapefun,
	                                 Drop_chunk_handler_fun cfun,
	                                 Drop_shape_handler_fun facefun,
	                                 void *udata);

	static void DestroyStudioDropDest(Windnd  *&windnd, HWND &hWnd);
};

/*
 * The IDropSource implementation
 */

class FAR Windropsource final :  public IDropSource {
private:
	std::atomic<DWORD> m_cRef;

	HWND drag_shape;
	HBITMAP drag_bitmap;
	int shw, shh;
	~Windropsource();

public:
	Windropsource(HBITMAP pdrag_bitmap, int x0, int y0);

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState) override;
	STDMETHOD(GiveFeedback)(DWORD dwEffect) override;

};

/*
 * The IDataObject implementation
 */
class FAR Winstudioobj final :  public IDataObject {
private:
	std::atomic<DWORD> m_cRef;

	HBITMAP drag_image;

	windragdata data;
	~Winstudioobj() = default;

public:
	Winstudioobj(const windragdata& pdata);

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

	STDMETHOD(GetData)(FORMATETC *pFormatetc, STGMEDIUM *pmedium) override;
	STDMETHOD(GetDataHere)(FORMATETC *pFormatetc, STGMEDIUM *pmedium) override;
	STDMETHOD(QueryGetData)(
	    FORMATETC *pFormatetc
	) override;
	STDMETHOD(GetCanonicalFormatEtc)(
	    FORMATETC *pFormatetcIn,
	    FORMATETC *pFormatetcOut
	) override;
	STDMETHOD(SetData)(
	    FORMATETC *pFormatetc,
	    STGMEDIUM *pmedium,
	    BOOL fRelease
	) override;
	STDMETHOD(EnumFormatEtc)(
	    DWORD dwDirection,
	    IEnumFORMATETC **ppenumFormatetc
	) override;
	STDMETHOD(DAdvise)(
	    FORMATETC *pFormatetc,
	    DWORD advf,
	    IAdviseSink *pAdvSink,
	    DWORD *pdwConnection
	) override;
	STDMETHOD(DUnadvise)(
	    DWORD dwConnection
	) override;
	STDMETHOD(EnumDAdvise)(
	    IEnumSTATDATA **ppenumAdvise
	) override;
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif

#endif
